#pragma once

#include <Winux/Core/Operations.h>
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
    enum class ProcessOption : std::uint32_t
    {
        None = 0,
        CreateNoWindow = 1u << 0,
        CreateNewConsole = 1u << 1,
        Detached = 1u << 2
    };

    /*
        @summary
        Stores a set of process creation flags.
    */
    class ProcessOptions
    {
    public:
        constexpr ProcessOptions() = default;
        constexpr ProcessOptions(const ProcessOption option)
            : value_(static_cast<std::uint32_t>(option))
        {
        }

        /*
            @summary
            Checks whether the given option is enabled.
        */
        constexpr bool contains(const ProcessOption option) const
        {
            return (value_ & static_cast<std::uint32_t>(option)) != 0;
        }

        /*
            @summary
            Checks whether every option in a set is enabled.
        */
        constexpr bool contains_all(const ProcessOptions options) const
        {
            return (value_ & options.value_) == options.value_;
        }

        /*
            @summary
            Adds a creation flag to the current set.
        */
        constexpr void add(const ProcessOption option)
        {
            value_ |= static_cast<std::uint32_t>(option);
        }

    private:
        std::uint32_t value_ = 0;
    };

    using create_process_result = Core::Result<std::uint32_t>;

    /*
        @summary
        Builder for process creation calls with optional startup flags.
    */
    class create_process_operation
        : public Core::Operation<create_process_result, ProcessOptions>
    {
    public:
        create_process_operation(action action)
            : Operation(std::move(action))
        {
        }

        /*
            @summary
            Prevents a console window from appearing when the process starts.
        */
        create_process_operation& no_window()
        {
            options().add(ProcessOption::CreateNoWindow);
            return *this;
        }

        /*
            @summary
            Creates the process with its own console.
        */
        create_process_operation& new_console()
        {
            options().add(ProcessOption::CreateNewConsole);
            return *this;
        }

        /*
            @summary
            Starts the process as a detached child process.
        */
        create_process_operation& detached()
        {
            options().add(ProcessOption::Detached);
            return *this;
        }
    };

    virtual ~IProcess() = default;

    /*
        @summary
        Finds all running processes matching a given executable name.

        @param name
        Process name to search for.
    */
    virtual Core::Result<std::vector<std::uint32_t>> find_processes(const std::wstring& name) = 0;

    /*
        @summary
        Finds the first running process matching a given executable name.

        @param name
        Process name to search for.
    */
    virtual Core::Result<std::optional<std::uint32_t>> find_process(const std::wstring& name) = 0;

    /*
        @summary
        Resolves the installation path for a running process.

        @param process_id
        Identifier of the process to inspect.
    */
    virtual Core::Result<std::filesystem::path> find_location(std::uint32_t process_id) = 0;

    /*
        @summary
        Resolves the directory containing the current or specified process executable.

        @param process_id
        Optional identifier of the process to inspect. When omitted, uses the current process.
    */
    virtual Core::Result<std::filesystem::path> get_executable_directory(
        std::optional<std::uint32_t> process_id = std::nullopt) = 0;

    /*
        @summary
        Checks whether a process is still running.

        @param process_id
        Identifier of the process to inspect.
    */
    virtual Core::Result<bool> is_running(std::uint32_t process_id) = 0;

    /*
        @summary
        Starts building a process creation request for the given application.

        @param application
        Path or command used to launch the process.
    */
    create_process_operation create_process(const std::wstring& application)
    {
        return create_process_operation(
            [this, application](ProcessOptions options)
            {
                return create_process_impl(application, options);
            });
    }

    /*
        @summary
        Requests the termination of a running process.

        @param process_id
        Identifier of the process to terminate.
    */
    virtual Core::Result<void> terminate_process(std::uint32_t process_id) = 0;

    /*
        @summary
        Immediately terminates a running process without giving it a chance to clean up.

        @param process_id
        Identifier of the process to terminate.
    */
    virtual Core::Result<void> force_terminate_process(std::uint32_t process_id) = 0;

protected:
    /*
        @summary
        Creates the actual process using the requested start options.

        @param application
        Path or command used to launch the process.

        @param options
        Configured process launch flags.
    */
    virtual create_process_result create_process_impl(
        const std::wstring& application,
        ProcessOptions options) = 0;
};

}
