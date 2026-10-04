#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Utils/Strings.h>

#include <array>
#include <cerrno>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

namespace Winux::Platform::Linux {

Contracts::ISystem& Linux::GetSystem()
{
    return *this;
}

Core::Result<std::wstring> Linux::GetUsername()
{
    errno = 0;
    if (const auto* user = getpwuid(geteuid()); user && user->pw_name && user->pw_name[0] != '\0')
    {
        return Core::Result<std::wstring>::Success(String::Utf8ToWide(user->pw_name));
    }

    return Core::Result<std::wstring>::Failure(
        "Unable to determine the current username (error " + std::to_string(errno) + ")");
}

Core::Result<std::wstring> Linux::GetHostName()
{
    std::array<char, 256> hostname{};
    errno = 0;
    if (gethostname(hostname.data(), hostname.size()) == 0 && hostname[0] != '\0' && hostname.back() == '\0')
    {
        return Core::Result<std::wstring>::Success(String::Utf8ToWide(hostname.data()));
    }

    return Core::Result<std::wstring>::Failure(
        "Unable to determine the host name (error " + std::to_string(errno) + ")");
}

}
