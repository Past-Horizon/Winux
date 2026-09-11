#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace String
{
    /*
        @summary
        Converts a wide string to its UTF-8 text representation.

        @param value
        Source string to convert.

        @returns
        The converted UTF-8 string.
    */
    std::string ToString(const std::wstring& value);

#ifdef _WIN32
    /*
        @summary
        Formats a Win32 DWORD value as a readable string.

        @param Value
        Numeric value to convert.

        @returns
        The value rendered as text.
    */
    std::string FromDwordToString(DWORD Value);
#endif
}