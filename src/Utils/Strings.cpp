#include <Cudev/Cudev.h>
#include <Winux/Utils/Strings.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

const char* CodecErrorMessage(Cudev::CodecError error) noexcept
{
    switch (error)
    {
    case Cudev::CodecError::InvalidLeadingByte:
        return "Invalid leading byte.";
    case Cudev::CodecError::InvalidContinuationByte:
        return "Invalid continuation byte.";
    case Cudev::CodecError::TruncatedSequence:
        return "Truncated encoded sequence.";
    case Cudev::CodecError::OverlongEncoding:
        return "Overlong encoding.";
    case Cudev::CodecError::SurrogateCodePoint:
        return "Surrogate code point is not valid Unicode text.";
    case Cudev::CodecError::CodePointOutOfRange:
        return "Code point is outside the Unicode range.";
    }
    return "Unknown Unicode codec error.";
}

template <typename Value>
Winux::Core::Result<Value> ToWinuxResult(
    const Cudev::Result<Value, Cudev::CodecError>& result)
{
    if (result.failed())
    {
        return Winux::Core::Result<Value>::failure(CodecErrorMessage(*result.error()));
    }
    return Winux::Core::Result<Value>::success(result.value());
}

Winux::Core::Result<void> ToWinuxResult(
    const Cudev::Result<void, Cudev::CodecError>& result)
{
    if (result.failed())
    {
        return Winux::Core::Result<void>::failure(CodecErrorMessage(*result.error()));
    }
    return Winux::Core::Result<void>::success();
}

}

namespace String
{

std::string ToString(const std::wstring& value)
{
    return WideToUtf8(value);
}

std::string WideToUtf8(const std::wstring& value)
{
    const auto result = Cudev::Wide::ToUtf8(value);
    return result.succeeded() ? result.value() : std::string{};
}

std::wstring Utf8ToWide(const std::string& value)
{
    const auto result = Cudev::Wide::FromUtf8(value);
    return result.succeeded() ? result.value() : std::wstring{};
}

Winux::Core::Result<std::string> ToUtf8(std::u16string_view value)
{
    return ToWinuxResult(Cudev::Convert::ToUtf8(value));
}

Winux::Core::Result<std::string> ToUtf8(std::u32string_view value)
{
    return ToWinuxResult(Cudev::Convert::ToUtf8(value));
}

Winux::Core::Result<std::string> ToUtf8(std::wstring_view value)
{
    return ToWinuxResult(Cudev::Wide::ToUtf8(value));
}

Winux::Core::Result<std::u16string> ToUtf16(std::string_view value)
{
    return ToWinuxResult(Cudev::Convert::ToUtf16(value));
}

Winux::Core::Result<std::u16string> ToUtf16(std::u32string_view value)
{
    return ToWinuxResult(Cudev::Convert::ToUtf16(value));
}

Winux::Core::Result<std::u16string> ToUtf16(std::wstring_view value)
{
    return ToWinuxResult(Cudev::Wide::ToUtf16(value));
}

Winux::Core::Result<std::u32string> ToUtf32(std::string_view value)
{
    return ToWinuxResult(Cudev::Convert::ToUtf32(value));
}

Winux::Core::Result<std::u32string> ToUtf32(std::u16string_view value)
{
    return ToWinuxResult(Cudev::Convert::ToUtf32(value));
}

Winux::Core::Result<std::u32string> ToUtf32(std::wstring_view value)
{
    return ToWinuxResult(Cudev::Wide::ToUtf32(value));
}

Winux::Core::Result<std::wstring> FromUtf8(std::string_view value)
{
    return ToWinuxResult(Cudev::Wide::FromUtf8(value));
}

Winux::Core::Result<std::wstring> FromUtf16(std::u16string_view value)
{
    return ToWinuxResult(Cudev::Wide::FromUtf16(value));
}

Winux::Core::Result<std::wstring> FromUtf32(std::u32string_view value)
{
    return ToWinuxResult(Cudev::Wide::FromUtf32(value));
}

Winux::Core::Result<std::string> EncodeUtf8(std::u32string_view value)
{
    return ToWinuxResult(Cudev::Utf8::U8Codec{}.Encode(value));
}

Winux::Core::Result<std::u32string> DecodeUtf8(std::string_view value)
{
    return ToWinuxResult(Cudev::Utf8::U8Codec{}.Decode(value));
}

Winux::Core::Result<void> ValidateUtf8(std::string_view value)
{
    return ToWinuxResult(Cudev::Utf8::U8Codec{}.Validate(value));
}

Winux::Core::Result<std::u16string> EncodeUtf16(std::u32string_view value)
{
    return ToWinuxResult(Cudev::Utf16::U16Codec{}.Encode(value));
}

Winux::Core::Result<std::u32string> DecodeUtf16(std::u16string_view value)
{
    return ToWinuxResult(Cudev::Utf16::U16Codec{}.Decode(value));
}

Winux::Core::Result<void> ValidateUtf16(std::u16string_view value)
{
    return ToWinuxResult(Cudev::Utf16::U16Codec{}.Validate(value));
}

Winux::Core::Result<std::u32string> EncodeUtf32(std::u32string_view value)
{
    return ToWinuxResult(Cudev::Utf32::U32Codec{}.Encode(value));
}

Winux::Core::Result<std::u32string> DecodeUtf32(std::u32string_view value)
{
    return ToWinuxResult(Cudev::Utf32::U32Codec{}.Decode(value));
}

Winux::Core::Result<void> ValidateUtf32(std::u32string_view value)
{
    return ToWinuxResult(Cudev::Utf32::U32Codec{}.Validate(value));
}

#ifdef _WIN32
    std::string FromDwordToString(DWORD value)
    {
        return std::to_string(value);
    }
#endif
}
