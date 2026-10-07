#include <Winux/Platform/Windows/Win32.h>
#include <Winux/Platform/Windows/Process/WProcessArchitecture.h>
#include <Winux/Platform/Windows/Process/WProcessUtils.h>

#include <windows.h>

#include <string>

namespace Winux::Platform::Windows::Process::Detail {

Contracts::IProcess::Architecture ArchitectureFromMachine(const std::uint16_t machine) noexcept
{
    using Architecture = Contracts::IProcess::Architecture;

    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        return Architecture::X86;
    case IMAGE_FILE_MACHINE_AMD64:
        return Architecture::X64;
    case IMAGE_FILE_MACHINE_ARM:
    case IMAGE_FILE_MACHINE_ARMNT:
        return Architecture::Arm;
    case IMAGE_FILE_MACHINE_ARM64:
        return Architecture::Arm64;
    default:
        return Architecture::Unknown;
    }
}

}

namespace Winux::Platform::Windows {

Core::Result<Contracts::IProcess::Architecture> Win32::GetArchitecture(
    const std::uint32_t process_id)
{
    using Architecture = Contracts::IProcess::Architecture;
    using IsWow64Process2Function = BOOL(WINAPI*)(HANDLE, USHORT*, USHORT*);

    if (process_id == 0)
    {
        return Core::Result<Architecture>::Failure(
            "Unable to determine process architecture: invalid process ID");
    }

    const HANDLE process = Process::OpenProcessHandle(
        PROCESS_QUERY_LIMITED_INFORMATION,
        process_id);
    if (process == nullptr)
    {
        const DWORD error = GetLastError();
        return Core::Result<Architecture>::Failure(
            "Unable to open process to determine architecture (error " +
            std::to_string(error) + ")");
    }

    const HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    const auto is_wow64_process2 = kernel == nullptr
        ? nullptr
        : reinterpret_cast<IsWow64Process2Function>(
            GetProcAddress(kernel, "IsWow64Process2"));

    USHORT machine = IMAGE_FILE_MACHINE_UNKNOWN;
    if (is_wow64_process2 != nullptr)
    {
        USHORT process_machine = IMAGE_FILE_MACHINE_UNKNOWN;
        USHORT native_machine = IMAGE_FILE_MACHINE_UNKNOWN;
        if (!is_wow64_process2(process, &process_machine, &native_machine))
        {
            const DWORD error = GetLastError();
            CloseHandle(process);
            return Core::Result<Architecture>::Failure(
                "Unable to determine process architecture (error " +
                std::to_string(error) + ")");
        }

        machine = process_machine == IMAGE_FILE_MACHINE_UNKNOWN
            ? native_machine
            : process_machine;
    }
    else
    {
        BOOL is_wow64 = FALSE;
        if (!IsWow64Process(process, &is_wow64))
        {
            const DWORD error = GetLastError();
            CloseHandle(process);
            return Core::Result<Architecture>::Failure(
                "Unable to determine process architecture (error " +
                std::to_string(error) + ")");
        }

        if (is_wow64)
        {
            machine = IMAGE_FILE_MACHINE_I386;
        }
        else
        {
            SYSTEM_INFO system_info{};
            GetNativeSystemInfo(&system_info);
            switch (system_info.wProcessorArchitecture)
            {
            case PROCESSOR_ARCHITECTURE_INTEL:
                machine = IMAGE_FILE_MACHINE_I386;
                break;
            case PROCESSOR_ARCHITECTURE_AMD64:
                machine = IMAGE_FILE_MACHINE_AMD64;
                break;
            case PROCESSOR_ARCHITECTURE_ARM:
                machine = IMAGE_FILE_MACHINE_ARMNT;
                break;
            case PROCESSOR_ARCHITECTURE_ARM64:
                machine = IMAGE_FILE_MACHINE_ARM64;
                break;
            default:
                break;
            }
        }
    }

    CloseHandle(process);
    return Core::Result<Architecture>::Success(
        Process::Detail::ArchitectureFromMachine(machine));
}

}