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

Contracts::IProcess& Linux::process()
{
    return *this;
}

Contracts::IProcess::ProcessOptions Linux::supported_features() const
{
    Contracts::IProcess::ProcessOptions features;
    features.add(Contracts::IProcess::ProcessOption::Detached);
    return features;
}

Core::Result<std::vector<std::uint32_t>> Linux::find_processes(const std::wstring& name)
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
    return Core::Result<std::vector<std::uint32_t>>::success(std::move(process_ids));
}

Core::Result<std::optional<std::uint32_t>> Linux::find_process(const std::wstring& name)
{
    const auto process_ids = find_processes(name);
    if (process_ids.failed())
    {
        return Core::Result<std::optional<std::uint32_t>>::failure(process_ids.message());
    }

    const auto& ids = process_ids.value();
    return Core::Result<std::optional<std::uint32_t>>::success(
        ids.empty() ? std::nullopt : std::optional<std::uint32_t>(ids.front()));
}

Core::Result<std::filesystem::path> Linux::find_location(const std::uint32_t process_id)
{
    std::error_code error;
    const auto location = std::filesystem::read_symlink(
        std::filesystem::path("/proc") / std::to_string(process_id) / "exe",
        error);

    if (error)
    {
        return Core::Result<std::filesystem::path>::failure(
            "Unable to find process location (error " + error.message() + ")");
    }

    return Core::Result<std::filesystem::path>::success(location);
}

Core::Result<std::filesystem::path> Linux::get_executable_directory(
    const std::optional<std::uint32_t> process_id)
{
    const std::filesystem::path executable = std::filesystem::path("/proc") /
        (process_id.has_value() ? std::to_string(process_id.value()) : "self") /
        "exe";
    std::error_code error;
    const auto location = std::filesystem::read_symlink(executable, error);
    if (error)
    {
        return Core::Result<std::filesystem::path>::failure(
            "Unable to find executable directory (error " + error.message() + ")");
    }

    return Core::Result<std::filesystem::path>::success(location.parent_path());
}

Core::Result<bool> Linux::is_running(const std::uint32_t process_id)
{
    if (process_id == 0)
    {
        return Core::Result<bool>::failure("Unable to check process: invalid process ID");
    }

    if (kill(static_cast<pid_t>(process_id), 0) == 0 || errno == EPERM)
    {
        return Core::Result<bool>::success(true);
    }

    if (errno == ESRCH)
    {
        return Core::Result<bool>::success(false);
    }

    return Core::Result<bool>::failure(
        "Unable to check process (error " + std::to_string(errno) + ")");
}

Core::Result<std::uint32_t> Linux::create_process_impl(
    const std::wstring& application,
    const Contracts::IProcess::ProcessOptions requested_features)
{
    if (!supported_features().contains_all(requested_features))
    {
        return Core::Result<std::uint32_t>::failure(
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
        return Core::Result<std::uint32_t>::failure("Unable to create process: empty application");
    }

    int execution_pipe[2]{};
    if (pipe(execution_pipe) != 0)
    {
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", errno, ")");
        return Core::Result<std::uint32_t>::failure(
            "Unable to create process (error " + std::to_string(errno) + ")");
    }

    const int flags = fcntl(execution_pipe[1], F_GETFD);
    if (flags == -1 || fcntl(execution_pipe[1], F_SETFD, flags | FD_CLOEXEC) == -1)
    {
        const int error = errno;
        close(execution_pipe[0]);
        close(execution_pipe[1]);
        return Core::Result<std::uint32_t>::failure(
            "Unable to create process (error " + std::to_string(error) + ")");
    }

    const pid_t process_id = fork();

    if (process_id < 0)
    {
        const int error = errno;
        close(execution_pipe[0]);
        close(execution_pipe[1]);
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", error, ")");
        return Core::Result<std::uint32_t>::failure(
            "Unable to create process (error " + std::to_string(error) + ")");
    }

    if (process_id == 0)
    {
        close(execution_pipe[0]);

        if (requested_features.contains(Contracts::IProcess::ProcessOption::Detached) &&
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
        (void)write(execution_pipe[1], &error, sizeof(error));
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
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", execution_error, ")");
        return Core::Result<std::uint32_t>::failure(
            "Unable to create process (error " + std::to_string(execution_error) + ")");
    }

    Logger::Log(Logger::Level::Info, "Created process with ID ", process_id);
    return Core::Result<std::uint32_t>::success(static_cast<std::uint32_t>(process_id));
}

Core::Result<void> Linux::terminate_process(const std::uint32_t process_id)
{
    if (process_id == 0)
    {
        return Core::Result<void>::failure("Unable to terminate process: invalid process ID");
    }

    if (kill(static_cast<pid_t>(process_id), SIGTERM) != 0)
    {
        Logger::Log(Logger::Level::Error, "Unable to terminate process (error ", errno, ")");
        return Core::Result<void>::failure(
            "Unable to terminate process (error " + std::to_string(errno) + ")");
    }

    int status = 0;
    if (waitpid(static_cast<pid_t>(process_id), &status, 0) == -1 && errno != ECHILD)
    {
        Logger::Log(Logger::Level::Error, "Unable to wait for process termination (error ", errno, ")");
        return Core::Result<void>::failure(
            "Unable to wait for process termination (error " + std::to_string(errno) + ")");
    }

    return Core::Result<void>::success();
}

Core::Result<void> Linux::force_terminate_process(const std::uint32_t process_id)
{
    if (process_id == 0)
    {
        return Core::Result<void>::failure("Unable to force terminate process: invalid process ID");
    }

    if (kill(static_cast<pid_t>(process_id), SIGKILL) != 0)
    {
        Logger::Log(Logger::Level::Error, "Unable to force terminate process (error ", errno, ")");
        return Core::Result<void>::failure(
            "Unable to force terminate process (error " + std::to_string(errno) + ")");
    }

    int status = 0;
    if (waitpid(static_cast<pid_t>(process_id), &status, 0) == -1 && errno != ECHILD)
    {
        Logger::Log(Logger::Level::Error, "Unable to wait for force terminated process (error ", errno, ")");
        return Core::Result<void>::failure(
            "Unable to wait for force terminated process (error " + std::to_string(errno) + ")");
    }

    return Core::Result<void>::success();
}

}
