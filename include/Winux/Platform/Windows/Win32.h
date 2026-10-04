#pragma once

#include <Winux/Platform/Windows/WindowsMacroCleanup.h>
#include <Winux/Contracts/IPlatform.h>
#include <iostream>

namespace Winux::Platform::Windows {

class Win32 final : public Contracts::IPlatform, public Contracts::IProcess, public Contracts::IFileSystem, public Contracts::IEnvironment, public Contracts::ISystem, public Contracts::ILocale, public Contracts::ITerminal, public Contracts::ICrypto {
public:
    ~Win32() override = default;

    /*
        @summary
        Returns the process interface provided by the Windows platform.
    */
    Contracts::IProcess& GetProcess() override;

    /*
        @summary
        Returns the environment interface provided by the Windows platform.
    */
    Contracts::IEnvironment& GetEnvironment() override;

    /*
        @summary
        Returns the current user's username on Windows.
    */
    Contracts::ISystem& GetSystem() override;

    /*
        @summary
        Returns the locale interface provided by Windows.
    */
    Contracts::ILocale& GetLocale() override;

    /*
        @summary
        Returns the local wall-clock time in 24-hour form with millisecond precision.
    */
    Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>> GetTime() override;

    /*
        @summary
        Returns the current local civil date as year, month, and day fields.
    */
    Core::Result<std::chrono::year_month_day> GetDate() override;

    /*
        @summary
        Returns the current local UTC offset in seconds, positive east of UTC.
    */
    Core::Result<std::chrono::seconds> GetTimezone() override;

    Core::Result<std::wstring> GetUsername() override;

    Core::Result<std::wstring> GetHostName() override;

    /*
        @summary
        Returns the file-system interface provided by the Windows platform.
    */
    Contracts::IFileSystem& GetFileSystem() override;

    /*
        @summary
        Returns the terminal interface provided by the Windows platform.
    */
    Contracts::ITerminal& GetTerminal() override;

    Contracts::ICrypto& GetCrypto() override { return *this; }

    /*
        @summary
        Executes a command line and captures the produced text output.
    */
    Core::Result<std::string> ExecuteCommand(const std::wstring& command_line) override;

    /*
        @summary
        Creates a named mutex using the platform-native implementation.

        @param name
        Unique mutex name.
    */
    Core::Result<std::unique_ptr<Contracts::IMutex>> CreateMutex(
        const std::wstring& name) override;

    /*
        @summary
        Reports the set of process features supported by this implementation.
    */
    Contracts::CapabilitySet SupportedFeatures() const override;

    /*
        @summary
        Returns the current user's home directory on Windows.
    */
    Core::Result<std::filesystem::path> Home() override;

    /*
        @summary
        Returns the current user's desktop directory on Windows.
    */
    Core::Result<std::filesystem::path> Desktop() override;

    /*
        @summary
        Returns the application data directory on Windows.
    */
    Core::Result<std::filesystem::path> AppData(
        Contracts::AppDataScope scope = Contracts::AppDataScope::Local) override;

    /*
        @summary
        Reads a Windows environment variable.

        @param name
        Environment variable name.
    */
    Core::Result<std::wstring> GetEnv(const std::wstring& name) override;

    /*
        @summary
        Sets a Windows environment variable.

        @param name
        Environment variable name.

        @param value
        Desired value to store.
    */
    Core::Result<void> SetEnv(
        const std::wstring& name,
        const std::wstring& value) override;

    /*
        @summary
        Removes a Windows environment variable.

        @param name
        Environment variable name.
    */
    Core::Result<void> UnsetEnv(const std::wstring& name) override;

    Core::Result<void> MoveFile(
        const std::filesystem::path& source,
        const std::filesystem::path& destination) override;

    /*
        @summary
        Lists the process IDs for all matching running processes.

        @param name
        Executable name to search for.
    */
    Core::Result<std::vector<std::uint32_t>> FindProcesses(const std::wstring& name) override;

    /*
        @summary
        Finds a single process ID for a matching executable.

        @param name
        Executable name to search for.
    */
    Core::Result<std::optional<std::uint32_t>> FindProcess(const std::wstring& name) override;

    /*
        @summary
        Resolves the executable path for the given process.

        @param process_id
        Identifier of the process.
    */
    Core::Result<std::filesystem::path> FindLocation(std::uint32_t process_id) override;

    Core::Result<std::filesystem::path> GetExecutableDirectory(
        std::optional<std::uint32_t> process_id = std::nullopt) override;

    /*
        @summary
        Checks whether the given process is still active.

        @param process_id
        Identifier of the process.
    */
    Core::Result<bool> IsRunning(std::uint32_t process_id) override;

    /*
        @summary
        Starts a process with the requested Win32 launch settings.

        @param application
        Path or command to launch.

        @param requested_features
        Flags describing how the process should start.
    */
    Core::Result<std::uint32_t> CreateProcess(
        const std::wstring& application,
        const Contracts::CapabilitySet& requested_features = {}) override;

    /*
        @summary
        Terminates the given process by identifier.

        @param process_id
        Identifier of the process to stop.
    */
    Core::Result<void> TerminateProcess(std::uint32_t process_id) override;

    /*
        @summary
        Immediately terminates the given process by identifier.

        @param process_id
        Identifier of the process to stop.
    */
    Core::Result<void> ForceTerminateProcess(std::uint32_t process_id) override;
};

}
