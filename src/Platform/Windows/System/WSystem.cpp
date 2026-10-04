#include <Winux/Platform/Windows/Win32.h>

#include <windows.h>

namespace Winux::Platform::Windows {

Contracts::ISystem& Win32::GetSystem()
{
    return *this;
}

Core::Result<std::wstring> Win32::GetUsername()
{
    DWORD size = 256;
    std::wstring username(size, L'\0');

    for (;;)
    {
        if (GetUserNameW(username.data(), &size))
        {
            username.resize(size > 0 ? size - 1 : 0);
            return Core::Result<std::wstring>::Success(std::move(username));
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return Core::Result<std::wstring>::Failure(
                "Unable to determine the current username (error " + std::to_string(GetLastError()) + ")");
        }

        username.resize(size);
    }
}

Core::Result<std::wstring> Win32::GetHostName()
{
    std::wstring hostname(MAX_COMPUTERNAME_LENGTH + 1, L'\0');
    DWORD size = static_cast<DWORD>(hostname.size());

    if (!GetComputerNameW(hostname.data(), &size))
    {
        return Core::Result<std::wstring>::Failure(
            "Unable to determine the host name (error " + std::to_string(GetLastError()) + ")");
    }

    hostname.resize(size);
    return Core::Result<std::wstring>::Success(std::move(hostname));
}

}
