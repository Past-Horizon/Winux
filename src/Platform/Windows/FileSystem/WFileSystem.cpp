#include <Winux/Platform/Windows/Win32.h>

#include <windows.h>
#include <KnownFolders.h>
#include <ShlObj.h>

#include <filesystem>
#include <string>
#include <utility>

namespace Winux::Platform::Windows {

namespace {

bool InvalidName(const std::wstring& name)
{
    return name.empty() || name.find(L'=') != std::wstring::npos;
}

Core::Result<std::filesystem::path> KnownFolderPath(const KNOWNFOLDERID& id)
{
    PWSTR raw_path = nullptr;
    const HRESULT result = SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &raw_path);
    if (FAILED(result))
    {
        return Core::Result<std::filesystem::path>::failure(
            "Unable to resolve known folder (error " + std::to_string(result) + ")");
    }

    const std::filesystem::path path(raw_path);
    CoTaskMemFree(raw_path);
    return Core::Result<std::filesystem::path>::success(path);
}

}

Contracts::IFileSystem& Win32::file_system()
{
    return *this;
}

Core::Result<std::filesystem::path> Win32::home()
{
    return KnownFolderPath(FOLDERID_Profile);
}

Core::Result<std::filesystem::path> Win32::desktop()
{
    return KnownFolderPath(FOLDERID_Desktop);
}

Core::Result<std::filesystem::path> Win32::app_data()
{
    return KnownFolderPath(FOLDERID_RoamingAppData);
}

Core::Result<std::filesystem::path> Win32::temp()
{
    std::wstring buffer(MAX_PATH, L'\0');
    const DWORD length = GetTempPathW(static_cast<DWORD>(buffer.size()), buffer.data());
    if (length == 0)
    {
        return Core::Result<std::filesystem::path>::failure(
            "Unable to resolve temporary folder (error " + std::to_string(GetLastError()) + ")");
    }

    if (length >= buffer.size())
    {
        buffer.resize(length + 1);
        const DWORD retry_length = GetTempPathW(
            static_cast<DWORD>(buffer.size()),
            buffer.data());
        if (retry_length == 0 || retry_length >= buffer.size())
        {
            return Core::Result<std::filesystem::path>::failure(
                "Unable to resolve temporary folder (error " + std::to_string(GetLastError()) + ")");
        }
        buffer.resize(retry_length);
    }
    else
    {
        buffer.resize(length);
    }

    return Core::Result<std::filesystem::path>::success(std::filesystem::path(buffer));
}

Core::Result<std::wstring> Win32::get_env(const std::wstring& name)
{
    if (InvalidName(name))
    {
        return Core::Result<std::wstring>::failure("Invalid environment variable name");
    }

    std::wstring buffer(256, L'\0');
    for (;;)
    {
        const DWORD length = GetEnvironmentVariableW(name.c_str(), buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            const DWORD error = GetLastError();
            return Core::Result<std::wstring>::failure(
                "Unable to get environment variable (error " + std::to_string(error) + ")");
        }
        if (length < buffer.size())
        {
            buffer.resize(length);
            return Core::Result<std::wstring>::success(std::move(buffer));
        }
        buffer.resize(length + 1);
    }
}

Core::Result<void> Win32::set_env(
    const std::wstring& name,
    const std::wstring& value)
{
    if (InvalidName(name))
    {
        return Core::Result<void>::failure("Invalid environment variable name");
    }

    if (!SetEnvironmentVariableW(name.c_str(), value.c_str()))
    {
        return Core::Result<void>::failure(
            "Unable to set environment variable (error " + std::to_string(GetLastError()) + ")");
    }

    return Core::Result<void>::success();
}

Core::Result<void> Win32::unset_env(const std::wstring& name)
{
    if (InvalidName(name))
    {
        return Core::Result<void>::failure("Invalid environment variable name");
    }

    if (!SetEnvironmentVariableW(name.c_str(), nullptr))
    {
        return Core::Result<void>::failure(
            "Unable to unset environment variable (error " + std::to_string(GetLastError()) + ")");
    }

    return Core::Result<void>::success();
}

}
