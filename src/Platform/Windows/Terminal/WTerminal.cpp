#include <Winux/Platform/General/Terminal.h>
#include <Winux/Platform/Windows/Win32.h>

#include <array>
#include <cstdio>
#include <memory>
#include <string>

namespace Winux::Platform::Windows {

Contracts::ITerminal& Win32::GetTerminal()
{
    return *this;
}

Core::Result<std::string> Win32::ExecuteCommand(const std::wstring& command_line)
{
    if (command_line.empty())
    {
        return Core::Result<std::string>::Failure(
            "Unable to execute command: command line was empty");
    }

    std::FILE* pipe = _wpopen(command_line.c_str(), L"r");
    if (pipe == nullptr)
    {
        return Core::Result<std::string>::Failure(
            "Unable to execute command: failed to open process pipe");
    }

    const std::string output = Winux::Platform::ReadPipe(pipe);
    const int status = _pclose(pipe);
    if (status != 0)
    {
        return Core::Result<std::string>::Failure(
            "Unable to execute command (exit code " + std::to_string(status) + ")");
    }

    return Core::Result<std::string>::Success(output);
}

}
