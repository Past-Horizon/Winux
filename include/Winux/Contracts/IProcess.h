#pragma once

#include <Winux/Contracts/Capabilities.h>
#include <Winux/Core/Results.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace Winux::Contracts {

/*
    @summary
    Flags used to tune how a new process is created.
*/
class IProcess {
public:
    struct CreateNoWindow
    {
        static constexpr CapabilityId Id = 1;
    };

    struct CreateNewConsole
    {
        static constexpr CapabilityId Id = 2;
    };

    struct Detached
    {
        static constexpr CapabilityId Id = 3;
    };

    using CreateProcessResult = Core::Result<std::uint32_t>;

    virtual ~IProcess() = default;

    /*
        @summary
        Finds all running processes matching a given executable name.

        @param name
        Process name to search for.
    */
    virtual Core::Result<std::vector<std::uint32_t>> FindProcesses(const std::wstring& name) = 0;

    /*
        @summary
        Finds the first running process matching a given executable name.

        @param name
        Process name to search for.
    */
    virtual Core::Result<std::optional<std::uint32_t>> FindProcess(const std::wstring& name) = 0;

    /*
        @summary
        Resolves the installation path for a running process.

        @param process_id
        Identifier of the process to inspect.
    */
    virtual Core::Result<std::filesystem::path> FindLocation(std::uint32_t process_id) = 0;

    /*
        @summary
        Resolves the directory containing the current or specified process executable.

        @param process_id
        Optional identifier of the process to inspect. When omitted, uses the current process.
    */
    virtual Core::Result<std::filesystem::path> GetExecutableDirectory(
        std::optional<std::uint32_t> process_id = std::nullopt) = 0;

    /*
        @summary
        Checks whether a process is still running.

        @param process_id
        Identifier of the process to inspect.
    */
    virtual Core::Result<bool> IsRunning(std::uint32_t process_id) = 0;

    /*
        @summary
        Starts a process with the given application and options.

        @param application
        Path or command used to launch the process.
    */
    virtual CreateProcessResult CreateProcess(
        const std::wstring& application,
        const CapabilitySet& options = {}) = 0;

    /*
        @summary
        Requests the termination of a running process.

        @param process_id
        Identifier of the process to terminate.
    */
    virtual Core::Result<void> TerminateProcess(std::uint32_t process_id) = 0;

    /*
        @summary
        Immediately terminates a running process without giving it a chance to clean up.

        @param process_id
        Identifier of the process to terminate.
    */
    virtual Core::Result<void> ForceTerminateProcess(std::uint32_t process_id) = 0;

};

}
