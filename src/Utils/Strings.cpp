#include <string>
#include <cstdint>
#include <climits>

#include <Winux/Utils/Strings.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace String
{

std::wstring ToWide(const char* value)
{
    return value ? std::wstring(value, value + std::char_traits<char>::length(value)) : std::wstring{};
}

std::string ToString(const std::wstring& value)
{
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i)
    {
        uint32_t cp = static_cast<uint32_t>(value[i]);

#if WCHAR_MAX <= 0xFFFF
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            if (i + 1 < value.size()) {
                uint32_t low = static_cast<uint32_t>(value[i + 1]);
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    ++i;
                } else {
                    cp = 0xFFFD;
                }
            } else {
                cp = 0xFFFD;
            }
        }
        else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            cp = 0xFFFD;
        }
#endif

        if (cp > 0x10FFFF) cp = 0xFFFD;

        if (cp < 0x80) {
            result.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
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