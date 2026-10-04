#include <Winux/Winux.h>

#ifdef _WIN32
#include <Winux/Platform/Windows/Win32.h>
#else
#include <Winux/Platform/Linux/Linux.h>
#endif

#include <mutex>
#include <stdexcept>

namespace {

std::mutex platform_mutex;
std::unique_ptr<Winux::Contracts::IPlatform> platform_instance;

std::unique_ptr<Winux::Contracts::IPlatform> CreatePlatform()
{
#ifdef _WIN32
    return std::make_unique<Winux::Platform::Windows::Win32>();
#else
    return std::make_unique<Winux::Platform::Linux::Linux>();
#endif
}

Winux::Contracts::IPlatform& GetInitializedPlatform()
{
    std::lock_guard lock(platform_mutex);
    if (!platform_instance)
    {
        throw std::logic_error("Winux platform has not been initialized");
    }
    return *platform_instance;
}

}

namespace Winux::Platform {

Core::Result<void> Initialize()
{
    std::lock_guard lock(::platform_mutex);
    if (::platform_instance)
    {
        return Core::Result<void>::Failure("Winux platform has already been initialized");
    }
    ::platform_instance = ::CreatePlatform();
    return Core::Result<void>::Success();
}

Core::Result<bool> IsInitialized()
{
    std::lock_guard lock(::platform_mutex);
    return Core::Result<bool>::Success(static_cast<bool>(::platform_instance));
}

Core::Result<void> Shutdown()
{
    std::lock_guard lock(::platform_mutex);
    ::platform_instance.reset();
    return Core::Result<void>::Success();
}

}

namespace Winux {

template <> Crypto& Get<Crypto>()
{
    return ::GetInitializedPlatform().GetCrypto();
}

template <> Environment& Get<Environment>()
{
    return ::GetInitializedPlatform().GetEnvironment();
}

template <> FileSystem& Get<FileSystem>()
{
    return ::GetInitializedPlatform().GetFileSystem();
}

template <> Locale& Get<Locale>()
{
    return ::GetInitializedPlatform().GetLocale();
}

template <> Network& Get<Network>()
{
    return ::GetInitializedPlatform().GetNetwork();
}

template <> Process& Get<Process>()
{
    return ::GetInitializedPlatform().GetProcess();
}

template <> PlatformContext& Get<PlatformContext>()
{
    return ::GetInitializedPlatform();
}

template <> System& Get<System>()
{
    return ::GetInitializedPlatform().GetSystem();
}

template <> Terminal& Get<Terminal>()
{
    return ::GetInitializedPlatform().GetTerminal();
}

}
