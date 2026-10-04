#include <Winux/Platform/General/Terminal.h>
#include <Winux/Platform/Linux/Linux.h>

#include <Winux/Utils/Strings.h>

#include <array>
#include <cstdio>
#include <memory>
#include <string>

namespace Winux::Platform::Linux {

Contracts::ITerminal& Linux::GetTerminal()
{
    return *this;
}

Core::Result<std::string> Linux::ExecuteCommand(const std::wstring& command_line)
{
    const std::string narrow_command_line = String::ToString(command_line);
    if (narrow_command_line.empty())
    {
        return Core::Result<std::string>::Failure(
            "Unable to execute command: command line was empty");
    }

    std::FILE* pipe = popen(narrow_command_line.c_str(), "r");
    if (pipe == nullptr)
    {
        return Core::Result<std::string>::Failure(
            "Unable to execute command: failed to open process pipe");
    }

    const std::string output = Winux::Platform::ReadPipe(pipe);
    const int status = pclose(pipe);
    if (status != 0)
    {
        return Core::Result<std::string>::Failure(
            "Unable to execute command (exit code " + std::to_string(status) + ")");
    }

    return Core::Result<std::string>::Success(output);
}

}
