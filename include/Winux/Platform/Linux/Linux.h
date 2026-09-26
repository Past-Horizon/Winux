#pragma once

#include <Winux/Contracts/IPlatform.h>

namespace Winux::Platform::Linux {

class Linux final : public Contracts::IPlatform, public Contracts::IProcess, public Contracts::IFileSystem, public Contracts::IEnvironment, public Contracts::ISystem, public Contracts::ILocale, public Contracts::ITerminal {
public:
    ~Linux() override = default;

    /*
        @summary
        Returns the process interface provided by the Linux platform.
    */
    Contracts::IProcess& process() override;

    /*
        @summary
        Returns the environment interface provided by the Linux platform.
    */
    Contracts::IEnvironment& environment() override;

    /*
        @summary
        Returns the current user's username on Linux or Android.
    */
    Contracts::ISystem& system() override;

    /*
        @summary
        Returns the locale interface provided by Linux.
    */
    Contracts::ILocale& locale() override;

    /*
        @summary
        Returns the local wall-clock time in 24-hour form with millisecond precision.
    */
    Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>> get_time() override;

    /*
        @summary
        Returns the current local civil date as year, month, and day fields.
    */
    Core::Result<std::chrono::year_month_day> get_date() override;

    /*
        @summary
        Returns the current local UTC offset in seconds, positive east of UTC.
    */
    Core::Result<std::chrono::seconds> get_timezone() override;

    Core::Result<std::wstring> get_username() override;

    /*
        @summary
        Returns the file-system interface provided by the Linux platform.
    */
    Contracts::IFileSystem& file_system() override;

    /*
        @summary
        Returns the terminal interface provided by the Linux platform.
    */
    Contracts::ITerminal& terminal() override;

    /*
        @summary
        Executes a command line and captures the produced text output.
    */
    Core::Result<std::string> execute_command(const std::wstring& command_line) override;

    /*
        @summary
        Creates a reusable command object backed by the terminal.
    */
    std::shared_ptr<Contracts::ITerminal::ICommand> create_command(
        ExecuteHandler execute_handler,
        CanExecuteHandler can_execute_handler = {}) override;

    /*
        @summary
        Creates a named mutex using the platform-native implementation.

        @param name
        Unique mutex name.
    */
    Core::Result<std::unique_ptr<Contracts::IMutex>> create_mutex(
        const std::wstring& name) override;

    /*
        @summary
        Reports the set of process features supported by this implementation.
    */
    Contracts::IProcess::ProcessOptions supported_features() const override;

    /*
        @summary
        Returns the current user's home directory on Linux.
    */
    Core::Result<std::filesystem::path> home() override;

    /*
        @summary
        Returns the current user's desktop directory on Linux.
    */
    Core::Result<std::filesystem::path> desktop() override;

    /*
        @summary
        Returns the application data directory on Linux.
    */
    Core::Result<std::filesystem::path> app_data_impl(
        Contracts::AppDataScope scope) override;

    /*
        @summary
        Returns the temporary directory used by Linux.
    */
    Core::Result<std::filesystem::path> temp() override;

    /*
        @summary
        Reads a Linux environment variable.

        @param name
        Environment variable name.
    */
    Core::Result<std::wstring> get_env(const std::wstring& name) override;

    /*
        @summary
        Sets a Linux environment variable.

        @param name
        Environment variable name.

        @param value
        Desired value to store.
    */
    Core::Result<void> set_env(
        const std::wstring& name,
        const std::wstring& value) override;

    /*
        @summary
        Removes a Linux environment variable.

        @param name
        Environment variable name.
    */
    Core::Result<void> unset_env(const std::wstring& name) override;

    /*
        @summary
        Reads a file using the supplied stream mode.
    */
    Core::Result<std::string> read_file(
        const std::filesystem::path& file,
        std::ios::openmode mode) override;

    /*
        @summary
        Writes text to a file using the supplied stream mode.
    */
    Core::Result<void> write_file(
        const std::filesystem::path& file,
        std::string_view contents,
        std::ios::openmode mode) override;

    Core::Result<void> move_file(
        const std::filesystem::path& source,
        const std::filesystem::path& destination) override;

    /*
        @summary
        Lists the process IDs for all matching running processes.

        @param name
        Executable name to search for.
    */
    Core::Result<std::vector<std::uint32_t>> find_processes(const std::wstring& name) override;

    /*
        @summary
        Finds a single process ID for a matching executable.

        @param name
        Executable name to search for.
    */
    Core::Result<std::optional<std::uint32_t>> find_process(const std::wstring& name) override;

    /*
        @summary
        Resolves the executable path for the given process.

        @param process_id
        Identifier of the process.
    */
    Core::Result<std::filesystem::path> find_location(std::uint32_t process_id) override;

    Core::Result<std::filesystem::path> get_executable_directory(
        std::optional<std::uint32_t> process_id = std::nullopt) override;

    /*
        @summary
        Checks whether the given process is still active.

        @param process_id
        Identifier of the process.
    */
    Core::Result<bool> is_running(std::uint32_t process_id) override;

    /*
        @summary
        Starts a process with the requested launch settings.

        @param application
        Path or command to launch.

        @param requested_features
        Flags describing how the process should start.
    */
    Core::Result<std::uint32_t> create_process_impl(
        const std::wstring& application,
        Contracts::IProcess::ProcessOptions requested_features) override;

    /*
        @summary
        Terminates the given process by identifier.

        @param process_id
        Identifier of the process to stop.
    */
    Core::Result<void> terminate_process(std::uint32_t process_id) override;

    /*
        @summary
        Immediately terminates the given process by identifier.

        @param process_id
        Identifier of the process to stop.
    */
    Core::Result<void> force_terminate_process(std::uint32_t process_id) override;
};

}
