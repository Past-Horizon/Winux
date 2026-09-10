#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Utils/Logger.h>
#include <Winux/Utils/Strings.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

namespace Winux::Platform::Linux {

namespace {

bool MatchesProcessName(const std::filesystem::path& process_directory, const std::string& name)
{
    std::ifstream command_name(process_directory / "comm");
    std::string command;
    if (command_name && std::getline(command_name, command) && command == name)
    {
        return true;
    }

    std::error_code error;
    const std::filesystem::path executable =
        std::filesystem::read_symlink(process_directory / "exe", error);
    return !error && executable.filename().string() == name;
}

}

Contracts::IProcess& Linux::process()
{
    return *this;
}

std::vector<std::uint32_t> Linux::find_processes(const std::wstring& name)
{
    const std::string process_name = String::ToString(name);
    std::vector<std::uint32_t> process_ids;

    for (const auto& entry : std::filesystem::directory_iterator("/proc"))
    {
        if (!entry.is_directory() || entry.path().filename().string().find_first_not_of("0123456789") != std::string::npos)
        {
            continue;
        }

        const std::uint32_t process_id = static_cast<std::uint32_t>(std::stoul(entry.path().filename().string()));
        if (MatchesProcessName(entry.path(), process_name))
        {
            process_ids.push_back(process_id);
        }
    }

    Logger::Log(
        Logger::Level::Info,
        process_ids.empty() ? "Was unable to find process" : "Found matching processes");
    return process_ids;
}

std::uint32_t Linux::find_process(const std::wstring& name)
{
    const std::vector<std::uint32_t> process_ids = find_processes(name);
    return process_ids.empty() ? 0 : process_ids.front();
}

std::uint32_t Linux::create_process(const std::wstring& application)
{
    std::istringstream command_line(String::ToString(application));
    std::vector<std::string> arguments{
        std::istream_iterator<std::string>{ command_line },
        std::istream_iterator<std::string>{ }
    };
    if (arguments.empty())
    {
        Logger::Log(Logger::Level::Error, "Unable to create process: empty application");
        return 0;
    }

    const pid_t process_id = fork();

    if (process_id < 0)
    {
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", errno, ")");
        return 0;
    }

    if (process_id == 0)
    {
        std::vector<char*> command_arguments;
        command_arguments.reserve(arguments.size() + 1);
        for (std::string& argument : arguments)
        {
            command_arguments.push_back(argument.data());
        }
        command_arguments.push_back(nullptr);

        execvp(command_arguments.front(), command_arguments.data());
        _exit(EXIT_FAILURE);
    }

    Logger::Log(Logger::Level::Info, "Created process with ID ", process_id);
    return static_cast<std::uint32_t>(process_id);
}

bool Linux::terminate_process(const std::uint32_t process_id)
{
    if (kill(static_cast<pid_t>(process_id), SIGTERM) != 0)
    {
        Logger::Log(Logger::Level::Error, "Unable to terminate process (error ", errno, ")");
        return false;
    }

    return true;
}

}
