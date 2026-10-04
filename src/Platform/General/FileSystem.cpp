#include <Winux/Contracts/IFileSystem.h>

#include <fstream>
#include <string>
#include <utility>

namespace Winux::Contracts {

Core::Result<std::filesystem::path> IFileSystem::Temp()
{
    std::error_code error;
    const auto path = std::filesystem::temp_directory_path(error);
    if (error)
    {
        return Core::Result<std::filesystem::path>::Failure(
            "Unable to resolve temporary directory: " + error.message());
    }

    return Core::Result<std::filesystem::path>::Success(path);
}

Core::Result<bool> IFileSystem::Exists(const std::filesystem::path& path)
{
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    return error
        ? Core::Result<bool>::Failure("Unable to check path: " + error.message())
        : Core::Result<bool>::Success(exists);
}

Core::Result<bool> IFileSystem::IsDirectory(const std::filesystem::path& path)
{
    std::error_code error;
    const bool is_directory = std::filesystem::is_directory(path, error);
    return error
        ? Core::Result<bool>::Failure("Unable to check directory: " + error.message())
        : Core::Result<bool>::Success(is_directory);
}

Core::Result<bool> IFileSystem::CreateDirectories(const std::filesystem::path& path)
{
    std::error_code error;
    const bool created = std::filesystem::create_directories(path, error);
    return error
        ? Core::Result<bool>::Failure("Unable to create directories: " + error.message())
        : Core::Result<bool>::Success(created);
}

Core::Result<bool> IFileSystem::Remove(const std::filesystem::path& path)
{
    std::error_code error;
    const bool removed = std::filesystem::remove(path, error);
    return error
        ? Core::Result<bool>::Failure("Unable to remove path: " + error.message())
        : Core::Result<bool>::Success(removed);
}

Core::Result<std::uintmax_t> IFileSystem::FileSize(const std::filesystem::path& file)
{
    std::error_code error;
    const auto size = std::filesystem::file_size(file, error);
    return error
        ? Core::Result<std::uintmax_t>::Failure("Unable to get file size: " + error.message())
        : Core::Result<std::uintmax_t>::Success(size);
}

Core::Result<std::string> IFileSystem::ReadFile(
    const std::filesystem::path& file,
    const std::ios::openmode mode)
{
    std::ifstream stream(file, mode | std::ios::in);
    if (!stream)
    {
        return Core::Result<std::string>::Failure(
            "Unable to open file for reading: " + file.string());
    }

    stream.seekg(0, std::ios::end);
    const std::streampos size = stream.tellg();
    if (size < 0)
    {
        return Core::Result<std::string>::Failure(
            "Unable to determine file size: " + file.string());
    }

    std::string contents(static_cast<std::size_t>(size), '\0');
    stream.seekg(0, std::ios::beg);
    if (!contents.empty())
    {
        stream.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    }

    if (!stream && !stream.eof())
    {
        return Core::Result<std::string>::Failure(
            "Unable to read file: " + file.string());
    }

    return Core::Result<std::string>::Success(std::move(contents));
}

Core::Result<void> IFileSystem::WriteFile(
    const std::filesystem::path& file,
    const std::string_view contents,
    const std::ios::openmode mode)
{
    std::ofstream stream(file, mode | std::ios::out);
    if (!stream)
    {
        return Core::Result<void>::Failure(
            "Unable to open file for writing: " + file.string());
    }

    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    stream.flush();
    if (!stream)
    {
        return Core::Result<void>::Failure(
            "Unable to write file: " + file.string());
    }

    return Core::Result<void>::Success();
}

}