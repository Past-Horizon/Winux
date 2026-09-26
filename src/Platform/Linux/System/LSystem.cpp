#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Utils/Strings.h>

#include <cerrno>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

namespace Winux::Platform::Linux {

Contracts::ISystem& Linux::system()
{
    return *this;
}

Core::Result<std::wstring> Linux::get_username()
{
    errno = 0;
    if (const auto* user = getpwuid(geteuid()); user && user->pw_name && user->pw_name[0] != '\0')
    {
        return Core::Result<std::wstring>::success(String::Utf8ToWide(user->pw_name));
    }

    return Core::Result<std::wstring>::failure(
        "Unable to determine the current username (error " + std::to_string(errno) + ")");
}

}
