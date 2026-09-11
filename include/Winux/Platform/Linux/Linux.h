#pragma once

#include <Winux/Contracts/IPlatform.h>

namespace Winux::Platform::Linux {

class Linux final : public Contracts::IPlatform, public Contracts::IProcess, public Contracts::IFileSystem {
public:
    ~Linux() override = default;

    /*
        @summary
        Returns the process interface provided by the Linux platform.
    */
    Contracts::IProcess& process() override;

    /*
        @summary
        Returns the file-system interface provided by the Linux platform.
    */
    Contracts::IFileSystem& file_system() override;

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
    Core::Result<std::filesystem::path> app_data() override;

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
};

}
