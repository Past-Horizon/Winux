#include <Winux/Platform/Platform.h>

#ifdef _WIN32
#include <Winux/Platform/Windows/Win32.h>
#else
#include <Winux/Platform/Linux/Linux.h>
#endif

namespace Winux::Platform {

std::unique_ptr<Contracts::IPlatform> create()
{
#ifdef _WIN32
    return std::make_unique<Windows::Win32>();
#else
    return std::make_unique<Linux::Linux>();
#endif
}

}
