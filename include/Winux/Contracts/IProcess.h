#pragma once

#include <Winux/Core/Operations.h>
#include <Winux/Core/Results.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace Winux::Contracts {

class IProcess {
public:
    enum class ProcessOption : std::uint32_t
    {
        None = 0,
        CreateNoWindow = 1u << 0,
        CreateNewConsole = 1u << 1
    };

    class ProcessOptions
    {
    public:
        constexpr ProcessOptions() = default;
        constexpr ProcessOptions(const ProcessOption option)
            : value_(static_cast<std::uint32_t>(option))
        {
        }

        constexpr bool contains(const ProcessOption option) const
        {
            return (value_ & static_cast<std::uint32_t>(option)) != 0;
        }

        constexpr bool contains_all(const ProcessOptions options) const
        {
            return (value_ & options.value_) == options.value_;
        }

        constexpr void add(const ProcessOption option)
        {
            value_ |= static_cast<std::uint32_t>(option);
        }

    private:
        std::uint32_t value_ = 0;
    };

    using create_process_result = Core::Result<std::uint32_t>;

    class create_process_operation
        : public Core::Operation<create_process_result, ProcessOptions>
    {
    public:
        create_process_operation(action action)
            : Operation(std::move(action))
        {
        }

        create_process_operation& no_window()
        {
            options().add(ProcessOption::CreateNoWindow);
            return *this;
        }

        create_process_operation& new_console()
        {
            options().add(ProcessOption::CreateNewConsole);
            return *this;
        }
    };

    virtual ~IProcess() = default;

    virtual Core::Result<std::vector<std::uint32_t>> find_processes(const std::wstring& name) = 0;
    virtual Core::Result<std::optional<std::uint32_t>> find_process(const std::wstring& name) = 0;
    virtual Core::Result<std::filesystem::path> find_location(std::uint32_t process_id) = 0;
    virtual Core::Result<bool> is_running(std::uint32_t process_id) = 0;
    create_process_operation create_process(const std::wstring& application)
    {
        return create_process_operation(
            [this, application](ProcessOptions options)
            {
                return create_process_impl(application, options);
            });
    }

    virtual Core::Result<void> terminate_process(std::uint32_t process_id) = 0;

protected:
    virtual create_process_result create_process_impl(
        const std::wstring& application,
        ProcessOptions options) = 0;
};

}
