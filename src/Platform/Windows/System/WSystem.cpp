#include <Winux/Platform/Windows/Win32.h>

#include <windows.h>

namespace Winux::Platform::Windows {

Contracts::ISystem& Win32::system()
{
    return *this;
}

Core::Result<std::wstring> Win32::get_username()
{
    DWORD size = 256;
    std::wstring username(size, L'\0');

    for (;;)
    {
        if (GetUserNameW(username.data(), &size))
        {
            username.resize(size > 0 ? size - 1 : 0);
            return Core::Result<std::wstring>::success(std::move(username));
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return Core::Result<std::wstring>::failure(
                "Unable to determine the current username (error " + std::to_string(GetLastError()) + ")");
        }

        username.resize(size);
    }
}

}
