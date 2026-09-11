#include <string>

#include <Winux/Utils/Strings.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace String
{
    std::string ToString(const std::wstring& value)
    {
        std::string result;
        result.reserve(value.size());
        for (const wchar_t character : value)
        {
            result.push_back(static_cast<char>(character));
        }
        return result;
    }

#ifdef _WIN32
    std::string FromDwordToString(DWORD value)
    {
        return std::to_string(value);
    }
#endif
}