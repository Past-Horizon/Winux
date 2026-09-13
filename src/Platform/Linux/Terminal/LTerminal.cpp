#include <Winux/Platform/General/Terminal.h>
#include <Winux/Platform/Linux/Linux.h>

#include <Winux/Utils/Strings.h>

#include <array>
#include <cstdio>
#include <memory>
#include <string>

namespace Winux::Platform::Linux {

Contracts::ITerminal& Linux::terminal()
{
    return *this;
}

Core::Result<std::string> Linux::execute_command(const std::wstring& command_line)
{
    const std::string narrow_command_line = String::ToString(command_line);
    if (narrow_command_line.empty())
    {
        return Core::Result<std::string>::failure(
            "Unable to execute command: command line was empty");
    }

    std::FILE* pipe = popen(narrow_command_line.c_str(), "r");
    if (pipe == nullptr)
    {
        return Core::Result<std::string>::failure(
            "Unable to execute command: failed to open process pipe");
    }

    const std::string output = Winux::Platform::detail::read_pipe(pipe);
    const int status = pclose(pipe);
    if (status != 0)
    {
        return Core::Result<std::string>::failure(
            "Unable to execute command (exit code " + std::to_string(status) + ")");
    }

    return Core::Result<std::string>::success(output);
}

std::shared_ptr<Contracts::ITerminal::ICommand> Linux::create_command(
    ExecuteHandler execute_handler,
    CanExecuteHandler can_execute_handler)
{
    return Winux::Platform::detail::make_terminal_command(
        std::move(execute_handler),
        std::move(can_execute_handler));
}

}
