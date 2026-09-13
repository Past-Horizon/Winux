#include <Winux/Platform/General/Terminal.h>
#include <Winux/Platform/Windows/Win32.h>

#include <Winux/Utils/Strings.h>

#include <array>
#include <cstdio>
#include <memory>
#include <string>

namespace Winux::Platform::Windows {

Contracts::ITerminal& Win32::terminal()
{
    return *this;
}

Core::Result<std::string> Win32::execute_command(const std::wstring& command_line)
{
    const std::string narrow_command_line = String::ToString(command_line);
    if (narrow_command_line.empty())
    {
        return Core::Result<std::string>::failure(
            "Unable to execute command: command line was empty");
    }

    std::FILE* pipe = _wpopen(narrow_command_line.c_str(), L"r");
    if (pipe == nullptr)
    {
        return Core::Result<std::string>::failure(
            "Unable to execute command: failed to open process pipe");
    }

    const std::string output = Winux::Platform::detail::read_pipe(pipe);
    const int status = _pclose(pipe);
    if (status != 0)
    {
        return Core::Result<std::string>::failure(
            "Unable to execute command (exit code " + std::to_string(status) + ")");
    }

    return Core::Result<std::string>::success(output);
}

std::shared_ptr<Contracts::ITerminal::ICommand> Win32::create_command(
    ExecuteHandler execute_handler,
    CanExecuteHandler can_execute_handler)
{
    return Winux::Platform::detail::make_terminal_command(
        std::move(execute_handler),
        std::move(can_execute_handler));
}

}
