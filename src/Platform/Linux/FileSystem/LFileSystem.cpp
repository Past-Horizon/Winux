#include <Winux/Platform/Linux/Linux.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace Winux::Platform::Linux {

namespace {

Core::Result<std::filesystem::path> HomePath(Linux& platform)
{
    const auto home = platform.get_env(L"HOME");
    if (home.failed())
    {
        return Core::Result<std::filesystem::path>::failure(home.message());
    }

    return Core::Result<std::filesystem::path>::success(std::filesystem::path(home.value()));
}

bool InvalidName(const std::wstring& name)
{
    return name.empty() || name.find(L'=') != std::wstring::npos;
}

}

Contracts::IFileSystem& Linux::file_system()
{
    return *this;
}

Core::Result<std::filesystem::path> Linux::home()
{
    return HomePath(*this);
}

Core::Result<std::filesystem::path> Linux::desktop()
{
    const auto home = HomePath(*this);
    return home.failed()
        ? Core::Result<std::filesystem::path>::failure(home.message())
        : Core::Result<std::filesystem::path>::success(home.value() / "Desktop");
}

Core::Result<std::filesystem::path> Linux::app_data_impl(Contracts::AppDataScope scope)
{
    const wchar_t* variable = L"XDG_DATA_HOME";
    const char* fallback = ".local";
    const char* fallback_leaf = "share";

    switch (scope)
    {
    case Contracts::AppDataScope::local:
        break;
    case Contracts::AppDataScope::local_low:
        variable = L"XDG_CACHE_HOME";
        fallback_leaf = "cache";
        break;
    case Contracts::AppDataScope::roaming:
        variable = L"XDG_CONFIG_HOME";
        fallback_leaf = "config";
        break;
    default:
        return Core::Result<std::filesystem::path>::failure("Unknown application data scope");
    }

    const auto xdg_home = get_env(variable);
    if (xdg_home.succeeded() && !xdg_home.value().empty())
    {
        return Core::Result<std::filesystem::path>::success(
            std::filesystem::path(xdg_home.value()));
    }

    const auto home = HomePath(*this);
    return home.failed()
        ? Core::Result<std::filesystem::path>::failure(home.message())
        : Core::Result<std::filesystem::path>::success(home.value() / fallback / fallback_leaf);
}

Core::Result<std::filesystem::path> Linux::temp()
{
    return Core::Result<std::filesystem::path>::success(std::filesystem::temp_directory_path());
}

Core::Result<std::string> Linux::read_file(
    const std::filesystem::path& file,
    const std::ios::openmode mode)
{
    std::ifstream stream(file, mode | std::ios::in);
    if (!stream)
    {
        return Core::Result<std::string>::failure(
            "Unable to open file for reading: " + file.string());
    }

    stream.seekg(0, std::ios::end);
    const std::streampos size = stream.tellg();
    if (size < 0)
    {
        return Core::Result<std::string>::failure(
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
        return Core::Result<std::string>::failure(
            "Unable to read file: " + file.string());
    }

    return Core::Result<std::string>::success(std::move(contents));
}

Core::Result<void> Linux::write_file(
    const std::filesystem::path& file,
    const std::string_view contents,
    const std::ios::openmode mode)
{
    std::ofstream stream(file, mode | std::ios::out);
    if (!stream)
    {
        return Core::Result<void>::failure(
            "Unable to open file for writing: " + file.string());
    }

    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    stream.flush();
    if (!stream)
    {
        return Core::Result<void>::failure(
            "Unable to write file: " + file.string());
    }

    return Core::Result<void>::success();
}

Core::Result<void> Linux::move_file(
    const std::filesystem::path& source,
    const std::filesystem::path& destination)
{
    if (std::rename(source.c_str(), destination.c_str()) != 0)
    {
        const int error = errno;
        return Core::Result<void>::failure(
            "Unable to move file from " + source.string() + " to " +
            destination.string() + " (error " + std::to_string(error) + ": " +
            std::strerror(error) + ")");
    }

    return Core::Result<void>::success();
}

}
