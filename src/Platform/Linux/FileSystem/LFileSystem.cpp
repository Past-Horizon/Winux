#include <Winux/Platform/Linux/Linux.h>

#include <Winux/Utils/Strings.h>

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace Winux::Platform::Linux {

namespace {

std::wstring ToWide(const std::string& value)
{
    return std::wstring(value.begin(), value.end());
}

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

Core::Result<std::filesystem::path> Linux::app_data()
{
    const auto xdg_data_home = get_env(L"XDG_DATA_HOME");
    if (xdg_data_home.succeeded() && !xdg_data_home.value().empty())
    {
        return Core::Result<std::filesystem::path>::success(
            std::filesystem::path(xdg_data_home.value()));
    }

    const auto home = HomePath(*this);
    return home.failed()
        ? Core::Result<std::filesystem::path>::failure(home.message())
        : Core::Result<std::filesystem::path>::success(home.value() / ".local" / "share");
}

Core::Result<std::filesystem::path> Linux::temp()
{
    return Core::Result<std::filesystem::path>::success(std::filesystem::temp_directory_path());
}

Core::Result<std::wstring> Linux::get_env(const std::wstring& name)
{
    if (InvalidName(name))
    {
        return Core::Result<std::wstring>::failure("Invalid environment variable name");
    }

    const std::string narrow_name = String::ToString(name);
    const char* value = std::getenv(narrow_name.c_str());
    if (value == nullptr)
    {
        return Core::Result<std::wstring>::failure(
            "Environment variable is not set: " + narrow_name);
    }

    return Core::Result<std::wstring>::success(ToWide(value));
}

Core::Result<void> Linux::set_env(
    const std::wstring& name,
    const std::wstring& value)
{
    if (InvalidName(name))
    {
        return Core::Result<void>::failure("Invalid environment variable name");
    }

    const std::string narrow_name = String::ToString(name);
    const std::string narrow_value = String::ToString(value);
    if (setenv(narrow_name.c_str(), narrow_value.c_str(), 1) != 0)
    {
        return Core::Result<void>::failure(
            "Unable to set environment variable (error " + std::to_string(errno) + ")");
    }

    return Core::Result<void>::success();
}

Core::Result<void> Linux::unset_env(const std::wstring& name)
{
    if (InvalidName(name))
    {
        return Core::Result<void>::failure("Invalid environment variable name");
    }

    const std::string narrow_name = String::ToString(name);
    if (unsetenv(narrow_name.c_str()) != 0)
    {
        return Core::Result<void>::failure(
            "Unable to unset environment variable (error " + std::to_string(errno) + ")");
    }

    return Core::Result<void>::success();
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

}
