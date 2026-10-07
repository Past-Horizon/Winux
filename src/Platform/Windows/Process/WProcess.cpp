#include <Winux/Platform/Windows/Win32.h>
#include <Winux/Utils/Logger.h>

#include <Winux/Platform/Windows/Process/WProcessUtils.h>

#include <windows.h>
#include <TlHelp32.h>

#include <algorithm>

namespace Winux::Platform::Windows {

Contracts::IProcess& Win32::GetProcess() {
	return *this;
}

Core::Result<std::uint32_t> Win32::GetCurrentProcessId()
{
    return Core::Result<std::uint32_t>::Success(::GetCurrentProcessId());
}

Contracts::CapabilitySet Win32::SupportedFeatures() const
{
    Contracts::CapabilitySet features;
    features.Add<Contracts::IProcess::CreateNoWindow>();
    features.Add<Contracts::IProcess::CreateNewConsole>();
    features.Add<Contracts::IProcess::Detached>();
    return features;
}

Core::Result<std::vector<std::uint32_t>> Win32::FindProcesses(const std::wstring& name)
{
    Logger::Log(Logger::Level::Info, "Starting process lookup");
    std::vector<std::uint32_t> process_ids;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        const DWORD error = GetLastError();
        Logger::Log(Logger::Level::Error, "Process snapshot failed (error ", error, ")");
        return Core::Result<std::vector<std::uint32_t>>::Failure(
            "Process snapshot failed (error " + std::to_string(error) + ")");
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    Logger::Log(Logger::Level::Info, "Starting process enumeration");
    if (!Process32FirstW(snapshot, &entry))
    {
        const DWORD error = GetLastError();
        CloseHandle(snapshot);
        if (error == ERROR_NO_MORE_FILES)
        {
            return Core::Result<std::vector<std::uint32_t>>::Success({});
        }
        return Core::Result<std::vector<std::uint32_t>>::Failure(
            "Unable to enumerate processes (error " + std::to_string(error) + ")");
    }

    Logger::Log(Logger::Level::Info, "Process enumeration started");
    do
    {
        if (_wcsicmp(entry.szExeFile, name.c_str()) == 0)
        {
            process_ids.push_back(entry.th32ProcessID);
        }
    } while (Process32NextW(snapshot, &entry));
    const DWORD enumeration_error = GetLastError();
    CloseHandle(snapshot);
    if (enumeration_error != ERROR_NO_MORE_FILES)
    {
        return Core::Result<std::vector<std::uint32_t>>::Failure(
            "Unable to enumerate processes (error " + std::to_string(enumeration_error) + ")");
    }

    std::sort(process_ids.begin(), process_ids.end());
    if (process_ids.empty())
    {
        Logger::Log(Logger::Level::Info, "Was unable to find process");
    }
    else
    {
        Logger::Log(Logger::Level::Info, "Found matching processes");
    }

    return Core::Result<std::vector<std::uint32_t>>::Success(std::move(process_ids));
}

Core::Result<std::optional<std::uint32_t>> Win32::FindProcess(const std::wstring& name)
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

Core::Result<std::filesystem::path> Win32::FindLocation(const std::uint32_t process_id)
{
    const HANDLE process = Process::OpenProcessHandle(
        PROCESS_QUERY_LIMITED_INFORMATION,
        process_id);
    if (process == nullptr)
    {
        const DWORD error = GetLastError();
        return Core::Result<std::filesystem::path>::Failure(
            "Unable to open process (error " + std::to_string(error) + ")");
    }

    std::wstring location(32768, L'\0');
    DWORD location_size = static_cast<DWORD>(location.size());
    const BOOL found = QueryFullProcessImageNameW(
        process,
        0,
        location.data(),
        &location_size);
    const DWORD error = found ? ERROR_SUCCESS : GetLastError();
    CloseHandle(process);

    if (!found)
    {
        return Core::Result<std::filesystem::path>::Failure(
            "Unable to find process location (error " + std::to_string(error) + ")");
    }

    location.resize(location_size);
    return Core::Result<std::filesystem::path>::Success(std::filesystem::path(location));
}

Core::Result<std::filesystem::path> Win32::GetExecutableDirectory(
    const std::optional<std::uint32_t> process_id)
{
    std::wstring location(32768, L'\0');
    DWORD location_size = static_cast<DWORD>(location.size());
    HANDLE process = nullptr;
    if (process_id.has_value())
    {
        process = Process::OpenProcessHandle(
            PROCESS_QUERY_LIMITED_INFORMATION,
            process_id.value());
        if (process == nullptr)
        {
            const DWORD error = GetLastError();
            return Core::Result<std::filesystem::path>::Failure(
                "Unable to open process (error " + std::to_string(error) + ")");
        }
    }

    BOOL found = FALSE;
    if (process_id.has_value())
    {
        found = QueryFullProcessImageNameW(process, 0, location.data(), &location_size);
    }
    else
    {
        const DWORD module_name_length = GetModuleFileNameW(
            nullptr,
            location.data(),
            location_size);
        found = module_name_length != 0;
        location_size = module_name_length;
    }
    const DWORD error = found ? ERROR_SUCCESS : GetLastError();
    if (process != nullptr)
    {
        CloseHandle(process);
    }

    if (!found)
    {
        return Core::Result<std::filesystem::path>::Failure(
            "Unable to find executable directory (error " + std::to_string(error) + ")");
    }

    location.resize(location_size);
    return Core::Result<std::filesystem::path>::Success(
        std::filesystem::path(location).parent_path());
}

Core::Result<bool> Win32::IsRunning(const std::uint32_t process_id)
{
    if (process_id == 0)
    {
        return Core::Result<bool>::Failure("Unable to check process: invalid process ID");
    }

    const HANDLE process = Process::OpenProcessHandle(SYNCHRONIZE, process_id);
    if (process == nullptr)
    {
        const DWORD error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER)
        {
            return Core::Result<bool>::Success(false);
        }
        if (error == ERROR_ACCESS_DENIED)
        {
            return Core::Result<bool>::Success(true);
        }

        return Core::Result<bool>::Failure(
            "Unable to open process (error " + std::to_string(error) + ")");
    }

    const DWORD state = WaitForSingleObject(process, 0);
    const DWORD error = state == WAIT_FAILED ? GetLastError() : ERROR_SUCCESS;
    CloseHandle(process);

    if (state == WAIT_TIMEOUT)
    {
        return Core::Result<bool>::Success(true);
    }

    if (state == WAIT_OBJECT_0)
    {
        return Core::Result<bool>::Success(false);
    }

    return Core::Result<bool>::Failure(
        "Unable to check process (error " + std::to_string(error) + ")");
}

}
