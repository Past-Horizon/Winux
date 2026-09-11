#pragma once

#include <Winux/Core/Results.h>

#include <filesystem>
#include <ios>
#include <string>
#include <string_view>

namespace Winux::Contracts {

/*
    @summary
    Provides access to common file-system and environment locations across platforms.
*/
class IFileSystem {
public:
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
    virtual Core::Result<std::filesystem::path> app_data() = 0;

    /*
        @summary
        Returns the temporary directory used by the system.
    */
    virtual Core::Result<std::filesystem::path> temp() = 0;

    /*
        @summary
        Reads the value of an environment variable.

        @param name
        Environment variable name.
    */
    virtual Core::Result<std::wstring> get_env(const std::wstring& name) = 0;

    /*
        @summary
        Sets an environment variable to a new value.

        @param name
        Environment variable name.

        @param value
        Value to set.
    */
    virtual Core::Result<void> set_env(
        const std::wstring& name,
        const std::wstring& value) = 0;

    /*
        @summary
        Removes an environment variable.

        @param name
        Environment variable name.
    */
    virtual Core::Result<void> unset_env(const std::wstring& name) = 0;

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
};

}
