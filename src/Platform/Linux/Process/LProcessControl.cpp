#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Platform/Linux/Process/LProcessUtils.h>
#include <Winux/Utils/Logger.h>
#include <Winux/Utils/Strings.h>

#include <chrono>
#include <cerrno>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#if __has_include(<linux/close_range.h>)
#include <linux/close_range.h>
#endif

namespace Winux::Platform::Linux {

namespace {

bool SplitCommandLine(const std::string& command_line, std::vector<std::string>& arguments)
{
    std::string argument;
    bool quoted = false;
    bool started = false;

    for (std::size_t index = 0; index < command_line.size(); ++index)
    {
        const char character = command_line[index];
        if (character == '\\' && index + 1 < command_line.size() &&
            (command_line[index + 1] == '"' || command_line[index + 1] == '\\'))
        {
            argument.push_back(command_line[++index]);
            started = true;
        }
        else if (character == '"')
        {
            quoted = !quoted;
            started = true;
        }
        else if (!quoted && std::isspace(static_cast<unsigned char>(character)))
        {
            if (started)
            {
                arguments.push_back(std::move(argument));
                argument.clear();
                started = false;
            }
        }
        else
        {
            argument.push_back(character);
            started = true;
        }
    }

    if (quoted)
    {
        return false;
    }
    if (started)
    {
        arguments.push_back(std::move(argument));
    }

    return true;
}

std::chrono::steady_clock::time_point MakeDeadline(
    const std::chrono::milliseconds timeout)
{
    using Clock = std::chrono::steady_clock;
    const auto now = Clock::now();
    const auto maximum_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::time_point::max() - now);
    return timeout >= maximum_timeout ? Clock::time_point::max() : now + timeout;
}

Core::Result<void> WaitForProcessExit(
    const pid_t process_id,
    const std::chrono::steady_clock::time_point deadline)
{
    for (;;)
    {
        int status = 0;
        const pid_t wait_result = waitpid(process_id, &status, WNOHANG);
        if (wait_result == process_id)
        {
            return Core::Result<void>::Success();
        }
        if (wait_result == -1 && errno != EINTR && errno != ECHILD)
        {
            const int error = errno;
            return Core::Result<void>::Failure(
                "Unable to wait for process termination (error " + std::to_string(error) + ")");
        }

        if (kill(process_id, 0) != 0)
        {
            const int error = errno;
            if (error == ESRCH)
            {
                return Core::Result<void>::Success();
            }
            if (error != EPERM)
            {
                return Core::Result<void>::Failure(
                    "Unable to check process termination (error " + std::to_string(error) + ")");
            }
        }

        if (Process::Detail::IsZombieProcess(process_id))
        {
            return Core::Result<void>::Success();
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return Core::Result<void>::Failure("Timed out waiting for process termination");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
    }
}

bool MarkDescriptorsCloseOnExec(const long maximum_descriptor)
{
#if defined(SYS_close_range) && defined(CLOSE_RANGE_CLOEXEC)
    if (syscall(SYS_close_range, 3U, ~0U, CLOSE_RANGE_CLOEXEC) == 0)
    {
        return true;
    }
#endif

    for (long descriptor = 3; descriptor < maximum_descriptor; ++descriptor)
    {
        if (fcntl(static_cast<int>(descriptor), F_SETFD, FD_CLOEXEC) == -1 && errno != EBADF)
        {
            return false;
        }
    }
    return true;
}

void ReapChild(const pid_t process_id)
{
    while (waitpid(process_id, nullptr, 0) == -1 && errno == EINTR)
    {
    }
}

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

    std::vector<std::string> arguments;
    if (!SplitCommandLine(String::ToString(application), arguments))
    {
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process: command line contains an unbalanced quote");
    }
    if (arguments.empty())
    {
        Logger::Log(Logger::Level::Error, "Unable to create process: empty application");
        return Core::Result<std::uint32_t>::Failure("Unable to create process: empty application");
    }

    std::vector<char*> command_arguments;
    command_arguments.reserve(arguments.size() + 1);
    for (std::string& argument : arguments)
    {
        command_arguments.push_back(argument.data());
    }
    command_arguments.push_back(nullptr);

    int execution_pipe[2]{};
    if (pipe2(execution_pipe, O_CLOEXEC) != 0)
    {
        const int error = errno;
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", error, ")");
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process (error " + std::to_string(error) + ")");
    }

    errno = 0;
    const long maximum_descriptor = sysconf(_SC_OPEN_MAX);
    if (maximum_descriptor < 0)
    {
        const int error = errno;
        close(execution_pipe[0]);
        close(execution_pipe[1]);
        return Core::Result<std::uint32_t>::Failure(
            "Unable to determine file descriptor limit (error " + std::to_string(error) + ")");
    }
    const bool detach = requested_features.Has<Contracts::IProcess::Detached>();
    const pid_t process_id = fork();

    if (process_id < 0)
    {
        const int error = errno;
        close(execution_pipe[0]);
        close(execution_pipe[1]);
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", error, ")");
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process (error " + std::to_string(error) + ")");
    }

    if (process_id == 0)
    {
        close(execution_pipe[0]);

        int child_error = 0;
        if (!MarkDescriptorsCloseOnExec(maximum_descriptor))
        {
            child_error = errno;
        }
        else if (detach && setsid() == -1)
        {
            child_error = errno;
        }
        else
        {
            execvp(command_arguments.front(), command_arguments.data());
            child_error = errno;
        }

        const char* error_data = reinterpret_cast<const char*>(&child_error);
        std::size_t error_bytes_written = 0;
        while (error_bytes_written < sizeof(child_error))
        {
            const ssize_t bytes_written = write(
                execution_pipe[1],
                error_data + error_bytes_written,
                sizeof(child_error) - error_bytes_written);
            if (bytes_written == -1 && errno == EINTR)
            {
                continue;
            }
            if (bytes_written <= 0)
            {
                break;
            }
            error_bytes_written += static_cast<std::size_t>(bytes_written);
        }
        _exit(127);
    }

    close(execution_pipe[1]);
    int execution_error = 0;
    std::size_t error_bytes_read = 0;
    int read_error = 0;
    char* error_data = reinterpret_cast<char*>(&execution_error);
    while (error_bytes_read < sizeof(execution_error))
    {
        const ssize_t bytes_read = read(
            execution_pipe[0],
            error_data + error_bytes_read,
            sizeof(execution_error) - error_bytes_read);
        if (bytes_read == 0)
        {
            break;
        }
        if (bytes_read == -1 && errno == EINTR)
        {
            continue;
        }
        if (bytes_read == -1)
        {
            read_error = errno;
            break;
        }
        error_bytes_read += static_cast<std::size_t>(bytes_read);
    }
    close(execution_pipe[0]);

    if (read_error != 0)
    {
        kill(process_id, SIGKILL);
        ReapChild(process_id);
        Logger::Log(Logger::Level::Error, "Unable to read process startup result (error ", read_error, ")");
        return Core::Result<std::uint32_t>::Failure(
            "Unable to read process startup result (error " + std::to_string(read_error) + ")");
    }

    if (error_bytes_read != 0)
    {
        if (error_bytes_read != sizeof(execution_error))
        {
            execution_error = EIO;
        }
        ReapChild(process_id);
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", execution_error, ")");
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process (error " + std::to_string(execution_error) + ")");
    }

    Logger::Log(Logger::Level::Info, "Created process with ID ", process_id);
    return Core::Result<std::uint32_t>::Success(static_cast<std::uint32_t>(process_id));
}

Core::Result<void> Linux::TerminateProcess(
    const std::uint32_t process_id,
    const std::chrono::milliseconds timeout)
{
    if (!Process::Detail::IsValidProcessId(process_id))
    {
        return Core::Result<void>::Failure("Unable to terminate process: invalid process ID");
    }
    if (timeout < std::chrono::milliseconds::zero())
    {
        return Core::Result<void>::Failure("Unable to terminate process: timeout cannot be negative");
    }

    const pid_t native_process_id = static_cast<pid_t>(process_id);
    if (kill(native_process_id, SIGTERM) != 0)
    {
        const int error = errno;
        Logger::Log(Logger::Level::Error, "Unable to terminate process (error ", error, ")");
        return Core::Result<void>::Failure(
            "Unable to terminate process (error " + std::to_string(error) + ")");
    }

    return WaitForProcessExit(native_process_id, MakeDeadline(timeout));
}

Core::Result<void> Linux::ForceTerminateProcess(
    const std::uint32_t process_id,
    const std::chrono::milliseconds timeout)
{
    if (!Process::Detail::IsValidProcessId(process_id))
    {
        return Core::Result<void>::Failure("Unable to force terminate process: invalid process ID");
    }
    if (timeout < std::chrono::milliseconds::zero())
    {
        return Core::Result<void>::Failure("Unable to force terminate process: timeout cannot be negative");
    }

    const pid_t native_process_id = static_cast<pid_t>(process_id);
    const auto descendants = Process::Detail::GetProcessDescendants(native_process_id);
    if (descendants.Failed())
    {
        return Core::Result<void>::Failure(descendants.Message());
    }

    for (const pid_t descendant : descendants.Value())
    {
        if (kill(descendant, SIGKILL) != 0)
        {
            const int error = errno;
            if (error != ESRCH)
            {
                Logger::Log(Logger::Level::Error, "Unable to force terminate descendant process (error ", error, ")");
                return Core::Result<void>::Failure(
                    "Unable to force terminate descendant process (error " + std::to_string(error) + ")");
            }
        }
    }

    if (kill(native_process_id, SIGKILL) != 0)
    {
        const int error = errno;
        Logger::Log(Logger::Level::Error, "Unable to force terminate process (error ", error, ")");
        return Core::Result<void>::Failure(
            "Unable to force terminate process (error " + std::to_string(error) + ")");
    }

    const auto deadline = MakeDeadline(timeout);
    for (const pid_t descendant : descendants.Value())
    {
        const auto waited = WaitForProcessExit(descendant, deadline);
        if (waited.Failed())
        {
            return waited;
        }
    }

    return WaitForProcessExit(native_process_id, deadline);
}

}
