#include <Winux/Platform/Linux/Linux.h>

#include <Winux/Utils/Strings.h>

#include <cerrno>
#include <cstdlib>
#include <string>

namespace Winux::Platform::Linux {

namespace {

std::wstring ToWide(const std::string& value)
{
    return std::wstring(value.begin(), value.end());
}

bool InvalidName(const std::wstring& name)
{
    return name.empty() || name.find(L'=') != std::wstring::npos;
}

}

Contracts::IEnvironment& Linux::environment()
{
    return *this;
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
