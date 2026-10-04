#pragma once

#include <Winux/Core/Results.h>

#include <chrono>
#include <string>
#include <string_view>

namespace Winux::Contracts {

/*
    @summary
    Exposes the current local date, time, and UTC offset as structured values.
*/
class ILocale {
public:
    virtual ~ILocale() = default;

    /*
        @summary
        Returns the current local wall-clock time with millisecond precision.
    */
    virtual Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>> GetTime() = 0;

    /*
        @summary
        Returns the current local civil date.
    */
    virtual Core::Result<std::chrono::year_month_day> GetDate() = 0;

    /*
        @summary
        Returns the current local UTC offset, positive east of UTC.
    */
    virtual Core::Result<std::chrono::seconds> GetTimezone() = 0;

    /*
        @summary
        Converts UTF-16 text to UTF-8.

        @param value
        UTF-16 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::string> ToUtf8(std::u16string_view value);

    /*
        @summary
        Converts UTF-32 text to UTF-8.

        @param value
        UTF-32 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::string> ToUtf8(std::u32string_view value);

    /*
        @summary
        Converts wide text to UTF-8.

        @param value
        Wide input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::string> ToUtf8(std::wstring_view value);

    /*
        @summary
        Converts UTF-8 text to UTF-16.

        @param value
        UTF-8 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u16string> ToUtf16(std::string_view value);

    /*
        @summary
        Converts UTF-32 text to UTF-16.

        @param value
        UTF-32 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u16string> ToUtf16(std::u32string_view value);

    /*
        @summary
        Converts wide text to UTF-16.

        @param value
        Wide input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u16string> ToUtf16(std::wstring_view value);

    /*
        @summary
        Converts UTF-8 text to UTF-32.

        @param value
        UTF-8 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u32string> ToUtf32(std::string_view value);

    /*
        @summary
        Converts UTF-16 text to UTF-32.

        @param value
        UTF-16 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u32string> ToUtf32(std::u16string_view value);

    /*
        @summary
        Converts wide text to UTF-32.

        @param value
        Wide input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u32string> ToUtf32(std::wstring_view value);

    /*
        @summary
        Converts UTF-8 text to wide text.

        @param value
        UTF-8 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::wstring> FromUtf8(std::string_view value);

    /*
        @summary
        Converts UTF-16 text to wide text.

        @param value
        UTF-16 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::wstring> FromUtf16(std::u16string_view value);

    /*
        @summary
        Converts UTF-32 text to wide text.

        @param value
        UTF-32 input text.

        @returns
        The converted text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::wstring> FromUtf32(std::u32string_view value);

    /*
        @summary
        Encodes UTF-32 text as UTF-8.

        @param value
        Unicode scalar values to encode.

        @returns
        The encoded text, or a failed result if a value is invalid.
    */
    virtual Core::Result<std::string> EncodeUtf8(std::u32string_view value);

    /*
        @summary
        Decodes UTF-8 text to UTF-32.

        @param value
        UTF-8 input text.

        @returns
        The decoded text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u32string> DecodeUtf8(std::string_view value);

    /*
        @summary
        Validates UTF-8 text.

        @param value
        UTF-8 input text.

        @returns
        A successful result for valid input, or a failed result otherwise.
    */
    virtual Core::Result<void> ValidateUtf8(std::string_view value);

    /*
        @summary
        Encodes UTF-32 text as UTF-16.

        @param value
        Unicode scalar values to encode.

        @returns
        The encoded text, or a failed result if a value is invalid.
    */
    virtual Core::Result<std::u16string> EncodeUtf16(std::u32string_view value);

    /*
        @summary
        Decodes UTF-16 text to UTF-32.

        @param value
        UTF-16 input text.

        @returns
        The decoded text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u32string> DecodeUtf16(std::u16string_view value);

    /*
        @summary
        Validates UTF-16 text.

        @param value
        UTF-16 input text.

        @returns
        A successful result for valid input, or a failed result otherwise.
    */
    virtual Core::Result<void> ValidateUtf16(std::u16string_view value);

    /*
        @summary
        Encodes Unicode scalar values as UTF-32.

        @param value
        Unicode scalar values to encode.

        @returns
        The encoded text, or a failed result if a value is invalid.
    */
    virtual Core::Result<std::u32string> EncodeUtf32(std::u32string_view value);

    /*
        @summary
        Decodes UTF-32 text to Unicode scalar values.

        @param value
        UTF-32 input text.

        @returns
        The decoded text, or a failed result if the input is invalid.
    */
    virtual Core::Result<std::u32string> DecodeUtf32(std::u32string_view value);

    /*
        @summary
        Validates UTF-32 text.

        @param value
        UTF-32 input text.

        @returns
        A successful result for valid input, or a failed result otherwise.
    */
    virtual Core::Result<void> ValidateUtf32(std::u32string_view value);

    /*
        @summary
        Converts valid UTF-8 text to a wide string. Invalid input returns an empty string.
    */
    virtual std::wstring Utf8ToWide(const std::string& input);

    /*
        @summary
        Converts a wide string to UTF-8. Invalid input returns an empty string.
    */
    virtual std::string WideToUtf8(const std::wstring& input);
};

}