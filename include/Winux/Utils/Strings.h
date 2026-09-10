#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace String
{
    std::string ToString(const std::wstring& value);

#ifdef _WIN32
    std::string FromDwordToString(DWORD Value);
#endif
}