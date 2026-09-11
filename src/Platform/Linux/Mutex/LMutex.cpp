#include <Winux/Platform/Linux/Linux.h>

#include <memory>

namespace Winux::Platform::Linux {

Core::Result<std::unique_ptr<Contracts::IMutex>> Linux::create_mutex(
    const std::wstring&)
{
    return Core::Result<std::unique_ptr<Contracts::IMutex>>::failure(
        "Named mutexes are not implemented on Linux yet");
}

}
