#include <Winux/Platform/Windows/Win32.h>
#include <Winux/Utils/Logger.h>
#include <Winux/Platform/Windows/Process/WProcessUtils.h>

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <chrono>
#include <string>

namespace Winux::Platform::Windows {

namespace {

struct WindowCloseContext
{
    DWORD process_id;
    bool found_window = false;
};

BOOL CALLBACK CloseProcessWindow(HWND window, LPARAM parameter)
{
    auto& context = *reinterpret_cast<WindowCloseContext*>(parameter);
    DWORD window_process_id = 0;
    GetWindowThreadProcessId(window, &window_process_id);
    if (window_process_id == context.process_id)
    {
        context.found_window = true;
        PostMessageW(window, WM_CLOSE, 0, 0);
    }

    return TRUE;
}

}

Core::Result<std::uint32_t> Win32::CreateProcess(
    const std::wstring& application,
    const Contracts::CapabilitySet& requested_features)
{
    if (!SupportedFeatures().ContainsAll(requested_features))
    {
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process: requested features are unsupported");
    }

    if (requested_features.Has<Contracts::IProcess::Detached>() &&
        requested_features.Has<Contracts::IProcess::CreateNewConsole>())
    {
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process: detached and new console options are incompatible");
    }

    if (requested_features.Has<Contracts::IProcess::CreateNoWindow>() &&
        (requested_features.Has<Contracts::IProcess::Detached>() ||
         requested_features.Has<Contracts::IProcess::CreateNewConsole>()))
    {
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process: no-window cannot be combined with detached or new-console options");
    }

    int argument_count = 0;
    LPWSTR* parsed_arguments = CommandLineToArgvW(application.c_str(), &argument_count);
    if (parsed_arguments == nullptr)
    {
        const DWORD error = GetLastError();
        return Core::Result<std::uint32_t>::Failure(
            "Unable to parse process command line (error " + std::to_string(error) + ")");
    }
    if (argument_count == 0 || parsed_arguments[0][0] == L'\0')
    {
        LocalFree(parsed_arguments);
        return Core::Result<std::uint32_t>::Failure("Unable to create process: empty application");
    }
    const std::wstring executable = parsed_arguments[0];
    LocalFree(parsed_arguments);

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info{};
    std::wstring command_line = application;
    if (!CreateProcessW(
            executable.c_str(),
            command_line.data(),
            nullptr,
            nullptr,
            FALSE,
              (requested_features.Has<Contracts::IProcess::CreateNoWindow>()
                  ? CREATE_NO_WINDOW
                  : 0) |
                 (requested_features.Has<Contracts::IProcess::CreateNewConsole>()
                     ? CREATE_NEW_CONSOLE
                     : 0) |
                 (requested_features.Has<Contracts::IProcess::Detached>()
                     ? DETACHED_PROCESS
                     : 0),
            nullptr,
            nullptr,
            &startup_info,
            &process_info))
    {
        const DWORD error = GetLastError();
        Logger::Log(Logger::Level::Error, "Unable to create process (error ", error, ")");
        return Core::Result<std::uint32_t>::Failure(
            "Unable to create process (error " + std::to_string(error) + ")");
    }

    const std::uint32_t process_id = process_info.dwProcessId;
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    Logger::Log(Logger::Level::Info, "Created process with ID ", process_id);
    return Core::Result<std::uint32_t>::Success(process_id);
}

Core::Result<void> Win32::TerminateProcess(
    const std::uint32_t process_id,
    const std::chrono::milliseconds timeout)
{
    if (process_id == 0)
    {
        return Core::Result<void>::Failure("Unable to terminate process: invalid process ID");
    }

    DWORD timeout_milliseconds = 0;
    if (!Process::TryGetTimeoutMilliseconds(timeout, timeout_milliseconds))
    {
        return Core::Result<void>::Failure("Unable to terminate process: invalid timeout");
    }

    const HANDLE process = Process::OpenProcessHandle(
        SYNCHRONIZE,
        process_id);
    if (process == nullptr)
    {
        const DWORD error = GetLastError();
        Logger::Log(Logger::Level::Error, "Unable to open process (error ", error, ")");
        return Core::Result<void>::Failure(
            "Unable to open process (error " + std::to_string(error) + ")");
    }

    WindowCloseContext context{ process_id };
    SetLastError(ERROR_SUCCESS);
    if (!EnumWindows(CloseProcessWindow, reinterpret_cast<LPARAM>(&context)))
    {
        DWORD error = GetLastError();
        if (error == ERROR_SUCCESS)
        {
            error = ERROR_GEN_FAILURE;
        }
        CloseHandle(process);
        return Core::Result<void>::Failure(
            "Unable to enumerate process windows (error " + std::to_string(error) + ")");
    }
    if (!context.found_window)
    {
        CloseHandle(process);
        return Core::Result<void>::Failure(
            "Unable to request graceful process termination: process has no top-level windows");
    }

    const DWORD wait_result = WaitForSingleObject(process, timeout_milliseconds);
    if (wait_result != WAIT_OBJECT_0)
    {
        const DWORD error = wait_result == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError();
        Logger::Log(Logger::Level::Error, "Unable to wait for process termination (error ", error, ")");
        CloseHandle(process);
        return Core::Result<void>::Failure(
            "Unable to wait for process termination (error " + std::to_string(error) + ")");
    }

    CloseHandle(process);
    return Core::Result<void>::Success();
}

Core::Result<void> Win32::ForceTerminateProcess(
    const std::uint32_t process_id,
    const std::chrono::milliseconds timeout)
{
    if (process_id == 0)
    {
        return Core::Result<void>::Failure(
            "Unable to force terminate process: invalid process ID");
    }

    DWORD timeout_milliseconds = 0;
    if (!Process::TryGetTimeoutMilliseconds(timeout, timeout_milliseconds))
    {
        return Core::Result<void>::Failure("Unable to force terminate process: invalid timeout");
    }

    const auto processes_result = Process::GetProcessTree();
    if (processes_result.Failed())
    {
        return Core::Result<void>::Failure(processes_result.Message());
    }

    const ULONGLONG deadline = GetTickCount64() + timeout_milliseconds;
    DWORD descendant_error = ERROR_SUCCESS;
    for (const auto& process : processes_result.Value())
    {
        if (process.process_id != process_id &&
            Process::IsDescendant(processes_result.Value(), process_id, process.process_id))
        {
            const ULONGLONG now = GetTickCount64();
            const DWORD remaining_timeout = now >= deadline
                ? 0
                : static_cast<DWORD>(std::min<ULONGLONG>(deadline - now, INFINITE - 1ULL));
            const DWORD error = Process::TerminateNativeProcess(
                process.process_id,
                remaining_timeout);
            if (error != ERROR_SUCCESS && error != ERROR_INVALID_PARAMETER &&
                descendant_error == ERROR_SUCCESS)
            {
                descendant_error = error;
            }
        }
    }

    const ULONGLONG now = GetTickCount64();
    const DWORD remaining_timeout = now >= deadline
        ? 0
        : static_cast<DWORD>(std::min<ULONGLONG>(deadline - now, INFINITE - 1ULL));
    const DWORD error = Process::TerminateNativeProcess(process_id, remaining_timeout);
    if (error != ERROR_SUCCESS)
    {
        Logger::Log(Logger::Level::Error, "Unable to force terminate process (error ", error, ")");
        return Core::Result<void>::Failure(
            "Unable to force terminate process (error " + std::to_string(error) + ")");
    }
    if (descendant_error != ERROR_SUCCESS)
    {
        Logger::Log(Logger::Level::Error, "Unable to force terminate descendant process (error ", descendant_error, ")");
        return Core::Result<void>::Failure(
            "Unable to force terminate descendant process (error " +
            std::to_string(descendant_error) + ")");
    }

    return Core::Result<void>::Success();
}

}
