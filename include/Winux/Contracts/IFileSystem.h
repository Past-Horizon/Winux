#pragma once

#include <Winux/Core/Results.h>

#include <cstdint>
#include <filesystem>
#include <ios>
#include <string>
#include <string_view>

namespace Winux::Contracts {

enum class AppDataScope {
    Local,
    LocalLow,
    Roaming
};

/*
    @summary
    Provides access to common file-system and environment locations across platforms.
*/
class IFileSystem {
public:
    using AppDataResult = Core::Result<std::filesystem::path>;

    virtual ~IFileSystem() = default;

    /*
        @summary
        Returns the current user's home directory.
    */
    virtual Core::Result<std::filesystem::path> Home() = 0;

    /*
        @summary
        Returns the desktop directory for the current user.
    */
    virtual Core::Result<std::filesystem::path> Desktop() = 0;

    /*
        @summary
        Returns the application data directory for the current user.
    */
    virtual AppDataResult AppData(AppDataScope scope = AppDataScope::Local) = 0;

    /*
        @summary
        Returns the temporary directory used by the system.
    */
    virtual Core::Result<std::filesystem::path> Temp();

    /*
        @summary
        Checks whether a path exists.
    */
    virtual Core::Result<bool> Exists(const std::filesystem::path& path);

    /*
        @summary
        Checks whether a path refers to a directory.
    */
    virtual Core::Result<bool> IsDirectory(const std::filesystem::path& path);

    /*
        @summary
        Creates a directory and any missing parent directories.

        @returns
        True if one or more directories were created.
    */
    virtual Core::Result<bool> CreateDirectories(const std::filesystem::path& path);

    /*
        @summary
        Removes a file or an empty directory.

        @returns
        True if a file or directory was removed.
    */
    virtual Core::Result<bool> Remove(const std::filesystem::path& path);

    /*
        @summary
        Returns the size of a file in bytes.
    */
    virtual Core::Result<std::uintmax_t> FileSize(const std::filesystem::path& file);

    /*
        @summary
        Reads the contents of a file.

        @param file
        Path to the file.

        @param mode
        File open mode to use.
    */
    virtual Core::Result<std::string> ReadFile(
        const std::filesystem::path& file,
        std::ios::openmode mode = std::ios::in | std::ios::binary);

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
    virtual Core::Result<void> WriteFile(
        const std::filesystem::path& file,
        std::string_view contents,
        std::ios::openmode mode = std::ios::out | std::ios::binary | std::ios::trunc);

    /*
        @summary
        Moves a file using the native platform file-system operation.

        @param source
        Path to the file to move.

        @param destination
        Path where the file should be moved. An existing destination is replaced.

        @note
        Cross-filesystem moves are not emulated with copy and delete.
    */
    virtual Core::Result<void> MoveFile(
        const std::filesystem::path& source,
        const std::filesystem::path& destination) = 0;

};

}
