#pragma once

#include <Winux/Core/Operations.h>
#include <Winux/Core/Results.h>

#include <filesystem>
#include <ios>
#include <string>
#include <string_view>

namespace Winux::Contracts {

enum class AppDataScope {
    local,
    local_low,
    roaming
};

/*
    @summary
    Provides access to common file-system and environment locations across platforms.
*/
class IFileSystem {
public:
    using app_data_result = Core::Result<std::filesystem::path>;

    class app_data_operation
        : public Core::Operation<app_data_result, AppDataScope>
    {
    public:
        app_data_operation(action action)
            : Operation(std::move(action))
        {
        }

        app_data_operation& local()
        {
            options() = AppDataScope::local;
            return *this;
        }

        app_data_operation& local_low()
        {
            options() = AppDataScope::local_low;
            return *this;
        }

        app_data_operation& roaming()
        {
            options() = AppDataScope::roaming;
            return *this;
        }
    };

    virtual ~IFileSystem() = default;

    /*
        @summary
        Returns the current user's home directory.
    */
    virtual Core::Result<std::filesystem::path> home() = 0;

    /*
        @summary
        Returns the desktop directory for the current user.
    */
    virtual Core::Result<std::filesystem::path> desktop() = 0;

    /*
        @summary
        Returns the application data directory for the current user.
    */
    app_data_operation app_data()
    {
        return app_data_operation(
            [this](AppDataScope scope)
            {
                return app_data_impl(scope);
            });
    }

    /*
        @summary
        Returns the temporary directory used by the system.
    */
    virtual Core::Result<std::filesystem::path> temp() = 0;

    /*
        @summary
        Reads the contents of a file.

        @param file
        Path to the file.

        @param mode
        File open mode to use.
    */
    virtual Core::Result<std::string> read_file(
        const std::filesystem::path& file,
        std::ios::openmode mode = std::ios::in | std::ios::binary) = 0;

    /*
        @summary
        Writes text to a file.

        @param file
        Path to the file.

        @param contents
        Data to write.

        @param mode
        File open mode to use.
    */
    virtual Core::Result<void> write_file(
        const std::filesystem::path& file,
        std::string_view contents,
        std::ios::openmode mode = std::ios::out | std::ios::binary | std::ios::trunc) = 0;

    /*
        @summary
        Moves a file using the native platform file-system operation.

        @param source
        Path to the file to move.

        @param destination
        Path where the file should be moved.

        @note
        Cross-filesystem moves are not emulated with copy and delete.
    */
    virtual Core::Result<void> move_file(
        const std::filesystem::path& source,
        const std::filesystem::path& destination) = 0;

protected:
    virtual app_data_result app_data_impl(AppDataScope scope) = 0;
};

}
