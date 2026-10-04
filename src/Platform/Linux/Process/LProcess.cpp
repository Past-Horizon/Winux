#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Utils/Logger.h>
#include <Winux/Utils/Strings.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
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

Contracts::IProcess& Linux::GetProcess()
{
    return *this;
}

Contracts::CapabilitySet Linux::SupportedFeatures() const
{
    Contracts::CapabilitySet features;
    features.Add<Contracts::IProcess::Detached>();
    return features;
}

Core::Result<std::vector<std::uint32_t>> Linux::FindProcesses(const std::wstring& name)
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
    return Core::Result<std::vector<std::uint32_t>>::Success(std::move(process_ids));
}

Core::Result<std::optional<std::uint32_t>> Linux::FindProcess(const std::wstring& name)
{
    const auto process_ids = FindProcesses(name);
    if (process_ids.Failed())
    {
        return Core::Result<std::optional<std::uint32_t>>::Failure(process_ids.Message());
    }

    const auto& ids = process_ids.Value();
    return Core::Result<std::optional<std::uint32_t>>::Success(
        ids.empty() ? std::nullopt : std::optional<std::uint32_t>(ids.front()));
}

Core::Result<std::filesystem::path> Linux::FindLocation(const std::uint32_t process_id)
{
    std::error_code error;
    const auto location = std::filesystem::read_symlink(
        std::filesystem::path("/proc") / std::to_string(process_id) / "exe",
        error);

    if (error)
    {
        return Core::Result<std::filesystem::path>::Failure(
            "Unable to find process location (error " + error.message() + ")");
    }

    return Core::Result<std::filesystem::path>::Success(location);
}

Core::Result<std::filesystem::path> Linux::GetExecutableDirectory(
    const std::optional<std::uint32_t> process_id)
{
    const std::filesystem::path executable = std::filesystem::path("/proc") /
        (process_id.has_value() ? std::to_string(process_id.value()) : "self") /
        "exe";
    std::error_code error;
    const auto location = std::filesystem::read_symlink(executable, error);
    if (error)
    {
        return Core::Result<std::filesystem::path>::Failure(
            "Unable to find executable directory (error " + error.message() + ")");
    }

    return Core::Result<std::filesystem::path>::Success(location.parent_path());
}

Core::Result<bool> Linux::IsRunning(const std::uint32_t process_id)
{
    if (process_id == 0)
    {
        return Core::Result<bool>::Failure("Unable to check process: invalid process ID");
    }

    if (kill(static_cast<pid_t>(process_id), 0) == 0 || errno == EPERM)
    {
        return Core::Result<bool>::Success(true);
    }

    if (errno == ESRCH)
    {
        return Core::Result<bool>::Success(false);
    }

    return Core::Result<bool>::Failure(
        "Unable to check GetProcess (error " + std::to_string(errno) + ")");
}

Core::Result<std::uint32_t> Linux::CreateProcess(
    const std::wstring& application,
    const Contracts::CapabilitySet& requested_features)
{
    if (!SupportedFeatures().ContainsAll(requested_features))
    {
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process: requested features are unsupported");
    }

    std::istringstream command_line(String::ToString(application));
    std::vector<std::string> arguments{
        std::istream_iterator<std::string>{ command_line },
        std::istream_iterator<std::string>{ }
    };
    if (arguments.empty())
    {
        Logger::Log(Logger::Level::Error, "Unable to create process: empty application");
        return Core::Result<std::uint32_t>::Failure("Unable to create process: empty application");
    }

    int execution_pipe[2]{};
    if (pipe(execution_pipe) != 0)
    {
        Logger::Log(Logger::Level::Error, "Unable to create GetProcess (error ", errno, ")");
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create GetProcess (error " + std::to_string(errno) + ")");
    }

    const int flags = fcntl(execution_pipe[1], F_GETFD);
    if (flags == -1 || fcntl(execution_pipe[1], F_SETFD, flags | FD_CLOEXEC) == -1)
    {
        const int error = errno;
        close(execution_pipe[0]);
        close(execution_pipe[1]);
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create GetProcess (error " + std::to_string(error) + ")");
    }

    const pid_t process_id = fork();

    if (process_id < 0)
    {
        const int error = errno;
        close(execution_pipe[0]);
        close(execution_pipe[1]);
        Logger::Log(Logger::Level::Error, "Unable to create GetProcess (error ", error, ")");
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create GetProcess (error " + std::to_string(error) + ")");
    }

    if (process_id == 0)
    {
        close(execution_pipe[0]);

        if (requested_features.Has<Contracts::IProcess::Detached>() &&
            setsid() == -1)
        {
            _exit(EXIT_FAILURE);
        }

        std::vector<char*> command_arguments;
        command_arguments.reserve(arguments.size() + 1);
        for (std::string& argument : arguments)
        {
            command_arguments.push_back(argument.data());
        }
        command_arguments.push_back(nullptr);

        execvp(command_arguments.front(), command_arguments.data());
        const int error = errno;
        if (write(execution_pipe[1], &error, sizeof(error)) !=
            static_cast<ssize_t>(sizeof(error)))
        {
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_FAILURE);
    }

    close(execution_pipe[1]);
    int execution_error = 0;
    const ssize_t bytes_read = read(
        execution_pipe[0],
        &execution_error,
        sizeof(execution_error));
    close(execution_pipe[0]);

    if (bytes_read > 0)
    {
        (void)waitpid(process_id, nullptr, 0);
        Logger::Log(Logger::Level::Error, "Unable to create GetProcess (error ", execution_error, ")");
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create GetProcess (error " + std::to_string(execution_error) + ")");
    }

    Logger::Log(Logger::Level::Info, "Created process with ID ", process_id);
    return Core::Result<std::uint32_t>::Success(static_cast<std::uint32_t>(process_id));
}

Core::Result<void> Linux::TerminateProcess(const std::uint32_t process_id)
{
    if (process_id == 0)
    {
        return Core::Result<void>::Failure("Unable to terminate process: invalid process ID");
    }

    if (kill(static_cast<pid_t>(process_id), SIGTERM) != 0)
    {
        Logger::Log(Logger::Level::Error, "Unable to terminate GetProcess (error ", errno, ")");
        return Core::Result<void>::Failure(
            "Unable to terminate GetProcess (error " + std::to_string(errno) + ")");
    }

    int status = 0;
    if (waitpid(static_cast<pid_t>(process_id), &status, 0) == -1 && errno != ECHILD)
    {
        Logger::Log(Logger::Level::Error, "Unable to wait for process termination (error ", errno, ")");
        return Core::Result<void>::Failure(
            "Unable to wait for process termination (error " + std::to_string(errno) + ")");
    }

    return Core::Result<void>::Success();
}

Core::Result<void> Linux::ForceTerminateProcess(const std::uint32_t process_id)
{
    if (process_id == 0)
    {
        return Core::Result<void>::Failure("Unable to force terminate process: invalid process ID");
    }

    if (kill(static_cast<pid_t>(process_id), SIGKILL) != 0)
    {
        Logger::Log(Logger::Level::Error, "Unable to force terminate GetProcess (error ", errno, ")");
        return Core::Result<void>::Failure(
            "Unable to force terminate GetProcess (error " + std::to_string(errno) + ")");
    }

    int status = 0;
    if (waitpid(static_cast<pid_t>(process_id), &status, 0) == -1 && errno != ECHILD)
    {
        Logger::Log(Logger::Level::Error, "Unable to wait for force terminated GetProcess (error ", errno, ")");
        return Core::Result<void>::Failure(
            "Unable to wait for force terminated GetProcess (error " + std::to_string(errno) + ")");
    }

    return Core::Result<void>::Success();
}

}
