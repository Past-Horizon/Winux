#include <Winux/Platform/General/Terminal.h>

#include <Winux/Contracts/ITerminal.h>

#include <algorithm>
#include <any>
#include <array>
#include <cstdio>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace Winux::Platform {

namespace detail {

std::string read_pipe(std::FILE* pipe)
{
    std::array<char, 4096> buffer{};
    std::string output;

    while (true)
    {
        const std::size_t bytes_read = std::fread(buffer.data(), sizeof(char), buffer.size(), pipe);
        if (bytes_read == 0)
        {
            break;
        }

        output.append(buffer.data(), bytes_read);
    }

    return output;
}

namespace {

class TerminalCommand final : public Contracts::ITerminal::ICommand {
public:
    using ExecuteHandler = Contracts::ITerminal::ExecuteHandler;
    using CanExecuteHandler = Contracts::ITerminal::CanExecuteHandler;

    TerminalCommand(ExecuteHandler execute_handler, CanExecuteHandler can_execute_handler)
        : execute_handler_(std::move(execute_handler)),
          can_execute_handler_(std::move(can_execute_handler))
    {
    }

    bool can_execute(const std::any& parameter = {}) const override
    {
        return can_execute_handler_ ? can_execute_handler_(parameter) : true;
    }

    Core::Result<void> execute(const std::any& parameter = {}) override
    {
        if (!execute_handler_)
        {
            return Core::Result<void>::failure("Unable to execute command: no handler was registered");
        }

        return execute_handler_(parameter);
    }

    void add_changed(const std::function<void()>& handler) override
    {
        if (handler)
        {
            can_execute_changed_handlers_.push_back(handler);
        }
    }

    void remove_changed(const std::function<void()>& handler) override
    {
        if (!handler)
        {
            return;
        }

        const auto it = std::find_if(
            can_execute_changed_handlers_.begin(),
            can_execute_changed_handlers_.end(),
            [&handler](const std::function<void()>& current)
            {
                return current.target_type() == handler.target_type();
            });

        if (it != can_execute_changed_handlers_.end())
        {
            can_execute_changed_handlers_.erase(it);
        }
    }

    void raise_changed() override
    {
        for (const auto& handler : can_execute_changed_handlers_)
        {
            if (handler)
            {
                handler();
            }
        }
    }

private:
    ExecuteHandler execute_handler_;
    CanExecuteHandler can_execute_handler_;
    std::vector<std::function<void()>> can_execute_changed_handlers_;
};

}

std::shared_ptr<Contracts::ITerminal::ICommand> make_terminal_command(
    Contracts::ITerminal::ExecuteHandler execute_handler,
    Contracts::ITerminal::CanExecuteHandler can_execute_handler)
{
    return std::make_shared<TerminalCommand>(std::move(execute_handler), std::move(can_execute_handler));
}

}

}
