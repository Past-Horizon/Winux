#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Platform/Linux/Process/LProcessUtils.h>
#include <Winux/Utils/Logger.h>
#include <Winux/Utils/Strings.h>

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace Winux::Platform::Linux {

namespace {

bool MatchesProcessName(const std::filesystem::path& process_directory, const std::string& name)
{
    constexpr std::size_t comm_limit = 15;
    std::ifstream command_name(process_directory / "comm");
    std::string command;
    if (command_name && std::getline(command_name, command) &&
        name.size() <= comm_limit && command == name)
    {
        return true;
    }

    std::error_code error;
    std::string executable = std::filesystem::read_symlink(
        process_directory / "exe",
        error).filename().string();
    if (!error)
    {
        constexpr std::string_view deleted_suffix = " (deleted)";
        if (executable.ends_with(deleted_suffix))
        {
            executable.resize(executable.size() - deleted_suffix.size());
        }
        return executable == name;
    }

    return name.size() > comm_limit &&
        command == name.substr(0, comm_limit);
}

}

Contracts::IProcess& Linux::GetProcess()
{
    return *this;
}

Core::Result<std::uint32_t> Linux::GetCurrentProcessId()
{
    return Core::Result<std::uint32_t>::Success(
        static_cast<std::uint32_t>(::getpid()));
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

    std::error_code error;
    std::filesystem::directory_iterator entry("/proc", error);
    if (error)
    {
        return Core::Result<std::vector<std::uint32_t>>::Failure(
            "Unable to enumerate processes (error " + error.message() + ")");
    }

    const std::filesystem::directory_iterator end;
    while (entry != end)
    {
        const std::string name = entry->path().filename().string();
        std::uint32_t process_id = 0;
        const auto [parsed_end, parse_error] = std::from_chars(
            name.data(),
            name.data() + name.size(),
            process_id);
        if (parse_error == std::errc{} && parsed_end == name.data() + name.size() &&
            Process::Detail::IsValidProcessId(process_id))
        {
            std::error_code directory_error;
            if (entry->is_directory(directory_error) && !directory_error &&
                MatchesProcessName(entry->path(), process_name))
            {
                process_ids.push_back(process_id);
            }
        }

        entry.increment(error);
        if (error)
        {
            return Core::Result<std::vector<std::uint32_t>>::Failure(
                "Unable to enumerate processes (error " + error.message() + ")");
        }
    }

    std::sort(process_ids.begin(), process_ids.end());
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
    if (!Process::Detail::IsValidProcessId(process_id))
    {
        return Core::Result<bool>::Failure("Unable to check process: invalid process ID");
    }

    const pid_t native_process_id = static_cast<pid_t>(process_id);
    int status = 0;
    pid_t wait_result = -1;
    do
    {
        wait_result = waitpid(native_process_id, &status, WNOHANG);
    }
    while (wait_result == -1 && errno == EINTR);

    if (wait_result == native_process_id)
    {
        return Core::Result<bool>::Success(false);
    }
    if (wait_result == -1 && errno != ECHILD)
    {
        const int error = errno;
        return Core::Result<bool>::Failure(
            "Unable to check process state (error " + std::to_string(error) + ")");
    }

    if (kill(native_process_id, 0) == 0)
    {
        return Core::Result<bool>::Success(!Process::Detail::IsZombieProcess(native_process_id));
    }

    const int error = errno;
    if (error == EPERM)
    {
        return Core::Result<bool>::Success(!Process::Detail::IsZombieProcess(native_process_id));
    }

    if (error == ESRCH)
    {
        return Core::Result<bool>::Success(false);
    }

    return Core::Result<bool>::Failure(
        "Unable to check process (error " + std::to_string(error) + ")");
}

}
