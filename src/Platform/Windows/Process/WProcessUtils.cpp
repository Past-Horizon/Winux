#include <Winux/Platform/Windows/Process/WProcessUtils.h>

namespace Winux::Platform::Windows::Process {

HANDLE OpenProcessHandle(const DWORD desired_access, const std::uint32_t process_id)
{
    return ::OpenProcess(desired_access, FALSE, process_id);
}

}
