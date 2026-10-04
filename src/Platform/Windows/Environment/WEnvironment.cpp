#include <Winux/Platform/Windows/Win32.h>

#include <windows.h>

#include <string>

namespace Winux::Platform::Windows {

namespace {

bool InvalidName(const std::wstring& name)
{
    return name.empty() || name.find(L'=') != std::wstring::npos;
}

}

Contracts::IEnvironment& Win32::GetEnvironment()
{
    return *this;
}

Core::Result<std::wstring> Win32::GetEnv(const std::wstring& name)
{
    if (InvalidName(name))
    {
        return Core::Result<std::wstring>::Failure("Invalid environment variable name");
    }

    std::wstring buffer(256, L'\0');
    for (;;)
    {
        const DWORD length = GetEnvironmentVariableW(name.c_str(), buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            const DWORD error = GetLastError();
            return Core::Result<std::wstring>::Failure(
                "Unable to get environment variable (error " + std::to_string(error) + ")");
        }
        if (length < buffer.size())
        {
            buffer.resize(length);
            return Core::Result<std::wstring>::Success(std::move(buffer));
        }
        buffer.resize(length + 1);
    }
}

Core::Result<void> Win32::SetEnv(
    const std::wstring& name,
    const std::wstring& value)
{
    if (InvalidName(name))
    {
        return Core::Result<void>::Failure("Invalid environment variable name");
    }

    if (!SetEnvironmentVariableW(name.c_str(), value.c_str()))
    {
        return Core::Result<void>::Failure(
            "Unable to set environment variable (error " + std::to_string(GetLastError()) + ")");
    }

    return Core::Result<void>::Success();
}

Core::Result<void> Win32::UnsetEnv(const std::wstring& name)
{
    if (InvalidName(name))
    {
        return Core::Result<void>::Failure("Invalid environment variable name");
    }

    if (!SetEnvironmentVariableW(name.c_str(), nullptr))
    {
        return Core::Result<void>::Failure(
            "Unable to unset environment variable (error " + std::to_string(GetLastError()) + ")");
    }

    return Core::Result<void>::Success();
}

}
