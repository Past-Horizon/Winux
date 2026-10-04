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
        return Core::Result<std::filesystem::path>::Failure(
            "Unable to resolve known folder (error " + std::to_string(result) + ")");
    }

    const std::filesystem::path path(raw_path);
    CoTaskMemFree(raw_path);
    return Core::Result<std::filesystem::path>::Success(path);
}

}

Contracts::IFileSystem& Win32::GetFileSystem()
{
    return *this;
}

Core::Result<std::filesystem::path> Win32::Home()
{
    return KnownFolderPath(FOLDERID_Profile);
}

Core::Result<std::filesystem::path> Win32::Desktop()
{
    return KnownFolderPath(FOLDERID_Desktop);
}

Core::Result<std::filesystem::path> Win32::AppData(Contracts::AppDataScope scope)
{
    switch (scope)
    {
    case Contracts::AppDataScope::Local:
        return KnownFolderPath(FOLDERID_LocalAppData);
    case Contracts::AppDataScope::LocalLow:
        return KnownFolderPath(FOLDERID_LocalAppDataLow);
    case Contracts::AppDataScope::Roaming:
        return KnownFolderPath(FOLDERID_RoamingAppData);
    }

    return Core::Result<std::filesystem::path>::Failure("Unknown application data scope");
}

Core::Result<void> Win32::MoveFile(
    const std::filesystem::path& source,
    const std::filesystem::path& destination)
{
    if (!MoveFileExW(source.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        return Core::Result<void>::Failure(
            "Unable to move file from " + source.string() + " to " +
            destination.string() + " (error " +
            std::to_string(GetLastError()) + ")");
    }

    return Core::Result<void>::Success();
}

}
