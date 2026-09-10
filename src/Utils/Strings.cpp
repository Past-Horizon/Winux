#include <string>

#include <Winux/Utils/Strings.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace String
{
    std::string ToString(const std::wstring& value)
    {
        return std::string(value.begin(), value.end());
    }

#ifdef _WIN32
    std::string FromDwordToString(DWORD value)
    {
        return std::to_string(value);
    }
#endif
}