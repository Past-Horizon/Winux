#include <Winux/Platform/Windows/Win32.h>
#include <Winux/Utils/Logger.h>

#include <Winux/Platform/Windows/Process/WProcessUtils.h>

#include <windows.h>
#include <TlHelp32.h>

namespace Winux::Platform::Windows {

Contracts::IProcess& Win32::process() {
	return *this;
}

Contracts::IProcess::ProcessOptions Win32::supported_features() const
{
    Contracts::IProcess::ProcessOptions features;
    features.add(Contracts::IProcess::ProcessOption::CreateNoWindow);
    features.add(Contracts::IProcess::ProcessOption::CreateNewConsole);
    return features;
}

Core::Result<std::vector<std::uint32_t>> Win32::find_processes(const std::wstring& name)
{
    Logger::Log(Logger::Level::Info, "Starting process lookup");
    std::vector<std::uint32_t> process_ids;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        Logger::Log(Logger::Level::Error, "Process snapshot failed (error ", GetLastError(), ")");
        return Core::Result<std::vector<std::uint32_t>>::failure(
            "Process snapshot failed (error " + std::to_string(GetLastError()) + ")");
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    Logger::Log(Logger::Level::Info, "Starting process enumeration");
    if (Process32FirstW(snapshot, &entry))
    {
        Logger::Log(Logger::Level::Info, "Process enumeration started");
        do
        {
            if (_wcsicmp(entry.szExeFile, name.c_str()) == 0)
            {
                process_ids.push_back(entry.th32ProcessID);
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    if (process_ids.empty())
    {
        Logger::Log(Logger::Level::Info, "Was unable to find process");
    }
    else
    {
        Logger::Log(Logger::Level::Info, "Found matching processes");
    }

    return Core::Result<std::vector<std::uint32_t>>::success(std::move(process_ids));
}

Core::Result<std::optional<std::uint32_t>> Win32::find_process(const std::wstring& name)
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

Core::Result<std::filesystem::path> Win32::find_location(const std::uint32_t process_id)
{
    const HANDLE process = Process::OpenProcessHandle(
        PROCESS_QUERY_LIMITED_INFORMATION,
        process_id);
    if (process == nullptr)
    {
        return Core::Result<std::filesystem::path>::failure(
            "Unable to open process (error " + std::to_string(GetLastError()) + ")");
    }

    std::wstring location(32768, L'\0');
    DWORD location_size = static_cast<DWORD>(location.size());
    const BOOL found = QueryFullProcessImageNameW(
        process,
        0,
        location.data(),
        &location_size);
    CloseHandle(process);

    if (!found)
    {
        return Core::Result<std::filesystem::path>::failure(
            "Unable to find process location (error " + std::to_string(GetLastError()) + ")");
    }

    location.resize(location_size);
    return Core::Result<std::filesystem::path>::success(std::filesystem::path(location));
}

Core::Result<bool> Win32::is_running(const std::uint32_t process_id)
{
    const HANDLE process = Process::OpenProcessHandle(SYNCHRONIZE, process_id);
    if (process == nullptr)
    {
        return Core::Result<bool>::failure(
            "Unable to open process (error " + std::to_string(GetLastError()) + ")");
    }

    const DWORD state = WaitForSingleObject(process, 0);
    CloseHandle(process);

    if (state == WAIT_TIMEOUT)
    {
        return Core::Result<bool>::success(true);
    }

    if (state == WAIT_OBJECT_0)
    {
        return Core::Result<bool>::success(false);
    }

    return Core::Result<bool>::failure(
        "Unable to check process (error " + std::to_string(GetLastError()) + ")");
}

Core::Result<std::uint32_t> Win32::create_process_impl(
    const std::wstring& application,
    const Contracts::IProcess::ProcessOptions requested_features)
{
    if (!supported_features().contains_all(requested_features))
    {
        return Core::Result<std::uint32_t>::failure(
            "Unable to create process: requested features are unsupported");
    }

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info{};
    std::wstring command_line = application;
    if (!CreateProcessW(
            nullptr,
            command_line.data(),
            nullptr,
            nullptr,
            FALSE,
              (requested_features.contains(Contracts::IProcess::ProcessOption::CreateNoWindow)
                  ? CREATE_NO_WINDOW
                  : 0) |
                 (requested_features.contains(Contracts::IProcess::ProcessOption::CreateNewConsole)
                     ? CREATE_NEW_CONSOLE
                     : 0),
            nullptr,
            nullptr,
            &startup_info,
            &process_info))
    {
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", GetLastError(), ")");
        return Core::Result<std::uint32_t>::failure(
            "Unable to create process (error " + std::to_string(GetLastError()) + ")");
    }

    const std::uint32_t process_id = process_info.dwProcessId;
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    Logger::Log(Logger::Level::Info, "Created process with ID ", process_id);
    return Core::Result<std::uint32_t>::success(process_id);
}

Core::Result<void> Win32::terminate_process(const std::uint32_t process_id)
{
    const HANDLE process = Process::OpenProcessHandle(
        PROCESS_TERMINATE | SYNCHRONIZE,
        process_id);
    if (process == nullptr)
    {
        Logger::Log(Logger::Level::Error, "Unable to open process (error ", GetLastError(), ")");
        return Core::Result<void>::failure(
            "Unable to open process (error " + std::to_string(GetLastError()) + ")");
    }

    if (TerminateProcess(process, 1) == FALSE)
    {
        Logger::Log(Logger::Level::Error, "Unable to terminate process (error ", GetLastError(), ")");
        CloseHandle(process);
        return Core::Result<void>::failure(
            "Unable to terminate process (error " + std::to_string(GetLastError()) + ")");
    }

    if (WaitForSingleObject(process, INFINITE) != WAIT_OBJECT_0)
    {
        Logger::Log(Logger::Level::Error, "Unable to wait for process termination (error ", GetLastError(), ")");
        CloseHandle(process);
        return Core::Result<void>::failure(
            "Unable to wait for process termination (error " + std::to_string(GetLastError()) + ")");
    }

    CloseHandle(process);
    return Core::Result<void>::success();
}

}
