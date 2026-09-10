#include <Winux/Platform/Windows/Win32.h>
#include <Winux/Utils/Logger.h>

#include <windows.h>
#include <TlHelp32.h>

namespace Winux::Platform::Windows {

Contracts::IProcess& Win32::process() {
	return *this;
}

std::vector<std::uint32_t> Win32::find_processes(const std::wstring& name)
{
    Logger::Log(Logger::Level::Info, "Starting process lookup");
    std::vector<std::uint32_t> process_ids;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        Logger::Log(Logger::Level::Error, "Process snapshot failed (error ", GetLastError(), ")");
        return process_ids;
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

    return process_ids;
}

std::uint32_t Win32::find_process(const std::wstring& name)
{
    const std::vector<std::uint32_t> process_ids = find_processes(name);
    return process_ids.empty() ? 0 : process_ids.front();
}

std::uint32_t Win32::create_process(const std::wstring& application)
{
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
            0,
            nullptr,
            nullptr,
            &startup_info,
            &process_info))
    {
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", GetLastError(), ")");
        return 0;
    }

    const std::uint32_t process_id = process_info.dwProcessId;
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    Logger::Log(Logger::Level::Info, "Created process with ID ", process_id);
    return process_id;
}

bool Win32::terminate_process(const std::uint32_t process_id)
{
    const HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, process_id);
    if (process == nullptr)
    {
        Logger::Log(Logger::Level::Error, "Unable to open process (error ", GetLastError(), ")");
        return false;
    }

    const bool terminated = TerminateProcess(process, 1) != FALSE;
    CloseHandle(process);

    if (!terminated)
    {
        Logger::Log(Logger::Level::Error, "Unable to terminate process (error ", GetLastError(), ")");
    }

    return terminated;
}

}
