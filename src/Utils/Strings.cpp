#include <string>
#include <cstdint>
#include <climits>
#include <iterator>
#include <limits>
#include <type_traits>

#include <Winux/Utils/Strings.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

#ifndef _WIN32

static_assert(sizeof(wchar_t) == 4,
    "WideToUtf8/Utf8ToWide require a 32-bit wchar_t outside Windows.");

constexpr char32_t MaxCodePoint = 0x10FFFF;
constexpr char32_t SurrogateMin = 0xD800;
constexpr char32_t SurrogateMax = 0xDFFF;

bool IsValidCodePoint(char32_t code_point) noexcept
{
    return code_point <= MaxCodePoint &&
           !(code_point >= SurrogateMin && code_point <= SurrogateMax);
}

bool AppendUtf8(char32_t code_point, std::string& output)
{
    if (!IsValidCodePoint(code_point))
    {
        return false;
    }

    if (code_point <= 0x7F)
    {
        output.push_back(static_cast<char>(code_point));
    }
    else if (code_point <= 0x7FF)
    {
        output.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
    else if (code_point <= 0xFFFF)
    {
        output.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
    else
    {
        output.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
    return true;
}

bool DecodeUtf8CodePoint(
    std::string::const_iterator& iterator,
    std::string::const_iterator end,
    char32_t& code_point)
{
    const auto first_byte = static_cast<unsigned char>(*iterator);
    std::size_t sequence_length = 0;
    char32_t value = 0;
    char32_t minimum_value = 0;

    if ((first_byte & 0x80) == 0x00)
    {
        sequence_length = 1;
        value = first_byte;
    }
    else if ((first_byte & 0xE0) == 0xC0)
    {
        sequence_length = 2;
        value = first_byte & 0x1F;
        minimum_value = 0x80;
    }
    else if ((first_byte & 0xF0) == 0xE0)
    {
        sequence_length = 3;
        value = first_byte & 0x0F;
        minimum_value = 0x800;
    }
    else if ((first_byte & 0xF8) == 0xF0)
    {
        sequence_length = 4;
        value = first_byte & 0x07;
        minimum_value = 0x10000;
    }
    else
    {
        return false;
    }

    if (static_cast<std::size_t>(std::distance(iterator, end)) < sequence_length)
    {
        return false;
    }

    auto cursor = iterator;
    ++cursor;
    for (std::size_t index = 1; index < sequence_length; ++index, ++cursor)
    {
        const auto continuation_byte = static_cast<unsigned char>(*cursor);
        if ((continuation_byte & 0xC0) != 0x80)
        {
            return false;
        }
        value = (value << 6) | (continuation_byte & 0x3F);
    }

    if (value < minimum_value || !IsValidCodePoint(value))
    {
        return false;
    }

    code_point = value;
    iterator = cursor;
    return true;
}

#endif

}

namespace String
{

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

std::string WideToUtf8(const std::wstring& input)
{
    if (input.empty())
    {
        return {};
    }

#ifdef _WIN32
    if (input.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
    {
        return {};
    }

    const int required_size = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()),
        nullptr, 0, nullptr, nullptr);
    if (required_size <= 0)
    {
        return {};
    }

    std::string output(static_cast<std::size_t>(required_size), '\0');
    if (WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()),
            output.data(), required_size, nullptr, nullptr) <= 0)
    {
        return {};
    }
    return output;
#else
    std::string output;
    output.reserve(input.size());
    for (const wchar_t wide_character : input)
    {
        const auto code_point = static_cast<char32_t>(
            static_cast<std::make_unsigned_t<wchar_t>>(wide_character));
        if (!AppendUtf8(code_point, output))
        {
            return {};
        }
    }
    return output;
#endif
}

std::wstring Utf8ToWide(const std::string& input)
{
    if (input.empty())
    {
        return {};
    }

#ifdef _WIN32
    if (input.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
    {
        return {};
    }

    const int required_size = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()),
        nullptr, 0);
    if (required_size <= 0)
    {
        return {};
    }

    std::wstring output(static_cast<std::size_t>(required_size), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()),
            output.data(), required_size) <= 0)
    {
        return {};
    }
    return output;
#else
    std::wstring output;
    output.reserve(input.size());
    auto iterator = input.begin();
    const auto end = input.end();
    while (iterator != end)
    {
        char32_t code_point = 0;
        if (!DecodeUtf8CodePoint(iterator, end, code_point))
        {
            return {};
        }
        output.push_back(static_cast<wchar_t>(code_point));
    }
    return output;
#endif
}

#ifdef _WIN32
    std::string FromDwordToString(DWORD value)
    {
        return std::to_string(value);
    }
#endif
}
