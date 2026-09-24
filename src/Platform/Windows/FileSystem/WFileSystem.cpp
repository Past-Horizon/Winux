#include <Winux/Platform/Windows/Win32.h>

#include <windows.h>
#include <KnownFolders.h>
#include <ShlObj.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
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

Core::Result<std::filesystem::path> Win32::app_data_impl(Contracts::AppDataScope scope)
{
    switch (scope)
    {
    case Contracts::AppDataScope::local:
        return KnownFolderPath(FOLDERID_LocalAppData);
    case Contracts::AppDataScope::local_low:
        return KnownFolderPath(FOLDERID_LocalAppDataLow);
    case Contracts::AppDataScope::roaming:
        return KnownFolderPath(FOLDERID_RoamingAppData);
    }

    return Core::Result<std::filesystem::path>::failure("Unknown application data scope");
}

Core::Result<std::filesystem::path> Win32::temp()
{
    return Core::Result<std::filesystem::path>::success(
        std::filesystem::temp_directory_path());
}

Core::Result<std::string> Win32::read_file(
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

Core::Result<void> Win32::write_file(
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

Core::Result<void> Win32::move_file(
    const std::filesystem::path& source,
    const std::filesystem::path& destination)
{
    if (!MoveFileExW(source.c_str(), destination.c_str(), 0))
    {
        return Core::Result<void>::failure(
            "Unable to move file from " + source.string() + " to " +
            destination.string() + " (error " +
            std::to_string(GetLastError()) + ")");
    }

    return Core::Result<void>::success();
}

}
