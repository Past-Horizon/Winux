#pragma once

#include <cstdint>

#include <windows.h>

namespace Winux::Platform::Windows::Process {

HANDLE OpenProcessHandle(DWORD desired_access, std::uint32_t process_id);

}