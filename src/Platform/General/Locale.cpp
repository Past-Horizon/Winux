#include <Winux/Platform/General/Locale.h>
#include <Winux/Contracts/ILocale.h>
#include <Winux/Utils/Strings.h>

namespace Winux::Platform {

std::chrono::hh_mm_ss<std::chrono::milliseconds> MakeLocalTime(
    const std::tm& local_time,
    std::chrono::milliseconds subseconds)
{
    const auto since_midnight = std::chrono::hours{local_time.tm_hour}
        + std::chrono::minutes{local_time.tm_min}
        + std::chrono::seconds{local_time.tm_sec}
        + subseconds;
    return std::chrono::hh_mm_ss<std::chrono::milliseconds>{since_midnight};
}

std::chrono::year_month_day MakeLocalDate(const std::tm& local_time)
{
    return std::chrono::year{local_time.tm_year + 1900}
        / std::chrono::month{static_cast<unsigned>(local_time.tm_mon + 1)}
        / std::chrono::day{static_cast<unsigned>(local_time.tm_mday)};
}

}

namespace Winux::Contracts {

Core::Result<std::string> ILocale::ToUtf8(std::u16string_view value)
{
    return ::String::ToUtf8(value);
}

Core::Result<std::string> ILocale::ToUtf8(std::u32string_view value)
{
    return ::String::ToUtf8(value);
}

Core::Result<std::string> ILocale::ToUtf8(std::wstring_view value)
{
    return ::String::ToUtf8(value);
}

Core::Result<std::u16string> ILocale::ToUtf16(std::string_view value)
{
    return ::String::ToUtf16(value);
}

Core::Result<std::u16string> ILocale::ToUtf16(std::u32string_view value)
{
    return ::String::ToUtf16(value);
}

Core::Result<std::u16string> ILocale::ToUtf16(std::wstring_view value)
{
    return ::String::ToUtf16(value);
}

Core::Result<std::u32string> ILocale::ToUtf32(std::string_view value)
{
    return ::String::ToUtf32(value);
}

Core::Result<std::u32string> ILocale::ToUtf32(std::u16string_view value)
{
    return ::String::ToUtf32(value);
}

Core::Result<std::u32string> ILocale::ToUtf32(std::wstring_view value)
{
    return ::String::ToUtf32(value);
}

Core::Result<std::wstring> ILocale::FromUtf8(std::string_view value)
{
    return ::String::FromUtf8(value);
}

Core::Result<std::wstring> ILocale::FromUtf16(std::u16string_view value)
{
    return ::String::FromUtf16(value);
}

Core::Result<std::wstring> ILocale::FromUtf32(std::u32string_view value)
{
    return ::String::FromUtf32(value);
}

Core::Result<std::string> ILocale::EncodeUtf8(std::u32string_view value)
{
    return ::String::EncodeUtf8(value);
}

Core::Result<std::u32string> ILocale::DecodeUtf8(std::string_view value)
{
    return ::String::DecodeUtf8(value);
}

Core::Result<void> ILocale::ValidateUtf8(std::string_view value)
{
    return ::String::ValidateUtf8(value);
}

Core::Result<std::u16string> ILocale::EncodeUtf16(std::u32string_view value)
{
    return ::String::EncodeUtf16(value);
}

Core::Result<std::u32string> ILocale::DecodeUtf16(std::u16string_view value)
{
    return ::String::DecodeUtf16(value);
}

Core::Result<void> ILocale::ValidateUtf16(std::u16string_view value)
{
    return ::String::ValidateUtf16(value);
}

Core::Result<std::u32string> ILocale::EncodeUtf32(std::u32string_view value)
{
    return ::String::EncodeUtf32(value);
}

Core::Result<std::u32string> ILocale::DecodeUtf32(std::u32string_view value)
{
    return ::String::DecodeUtf32(value);
}

Core::Result<void> ILocale::ValidateUtf32(std::u32string_view value)
{
    return ::String::ValidateUtf32(value);
}

std::wstring ILocale::Utf8ToWide(const std::string& input)
{
    return ::String::Utf8ToWide(input);
}

std::string ILocale::WideToUtf8(const std::wstring& input)
{
    return ::String::WideToUtf8(input);
}

}