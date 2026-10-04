#include <Winux/Platform/Linux/Linux.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <string>

namespace Winux::Platform::Linux {

namespace {

Core::Result<std::filesystem::path> HomePath(Linux& platform)
{
    const auto home = platform.GetEnv(L"HOME");
    if (home.Failed())
    {
        return Core::Result<std::filesystem::path>::Failure(home.Message());
    }

    return Core::Result<std::filesystem::path>::Success(std::filesystem::path(home.Value()));
}

bool InvalidName(const std::wstring& name)
{
    return name.empty() || name.find(L'=') != std::wstring::npos;
}

}

Contracts::IFileSystem& Linux::GetFileSystem()
{
    return *this;
}

Core::Result<std::filesystem::path> Linux::Home()
{
    return HomePath(*this);
}

Core::Result<std::filesystem::path> Linux::Desktop()
{
    const auto home = HomePath(*this);
    return home.Failed()
        ? Core::Result<std::filesystem::path>::Failure(home.Message())
        : Core::Result<std::filesystem::path>::Success(home.Value() / "Desktop");
}

Core::Result<std::filesystem::path> Linux::AppData(Contracts::AppDataScope scope)
{
    const wchar_t* variable = L"XDG_DATA_HOME";
    const char* fallback = ".local";
    const char* fallback_leaf = "share";

    switch (scope)
    {
    case Contracts::AppDataScope::Local:
        break;
    case Contracts::AppDataScope::LocalLow:
        variable = L"XDG_CACHE_HOME";
        fallback_leaf = "cache";
        break;
    case Contracts::AppDataScope::Roaming:
        variable = L"XDG_CONFIG_HOME";
        fallback_leaf = "config";
        break;
    default:
        return Core::Result<std::filesystem::path>::Failure("Unknown application data scope");
    }

    const auto xdg_home = GetEnv(variable);
    if (xdg_home.Succeeded() && !xdg_home.Value().empty())
    {
        return Core::Result<std::filesystem::path>::Success(
            std::filesystem::path(xdg_home.Value()));
    }

    const auto home = HomePath(*this);
    return home.Failed()
        ? Core::Result<std::filesystem::path>::Failure(home.Message())
        : Core::Result<std::filesystem::path>::Success(home.Value() / fallback / fallback_leaf);
}

Core::Result<void> Linux::MoveFile(
    const std::filesystem::path& source,
    const std::filesystem::path& destination)
{
    if (std::rename(source.c_str(), destination.c_str()) != 0)
    {
        const int error = errno;
        return Core::Result<void>::Failure(
            "Unable to move file from " + source.string() + " to " +
            destination.string() + " (error " + std::to_string(error) + ": " +
            std::strerror(error) + ")");
    }

    return Core::Result<void>::Success();
}

}
