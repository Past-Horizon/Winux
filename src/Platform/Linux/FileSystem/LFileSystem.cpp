#include <Winux/Platform/Linux/Linux.h>

#include <Winux/Utils/Strings.h>

#include <cerrno>
#include <cstdlib>
#include <filesystem>
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

}
