#include <Winux/Platform/Windows/Win32.h>
#include <Winux/Platform/Windows/Process/WProcessUtils.h>

#include <windows.h>

#include <limits>
#include <string>

namespace Winux::Platform::Windows {

namespace {

bool IsValidRange(const std::uintptr_t address, const std::size_t size)
{
    if (size == 0)
    {
        return true;
    }

    return address != 0 &&
        static_cast<std::uintptr_t>(size - 1) <=
            (std::numeric_limits<std::uintptr_t>::max)() - address;
}

std::string TransferMessage(
    const char* operation,
    const std::size_t transferred,
    const std::size_t requested,
    const DWORD error)
{
    return std::string(operation) + " transferred " + std::to_string(transferred) +
        " of " + std::to_string(requested) + " bytes (error " +
        std::to_string(error) + ")";
}

}

Contracts::IMemory& Win32::GetMemory()
{
    return *this;
}

Core::Result<std::size_t> Win32::ReadMemory(
    const std::uint32_t process_id,
    const std::uintptr_t remote_address,
    const std::span<std::byte> output)
{
    if (output.empty())
    {
        return Core::Result<std::size_t>::Success(0);
    }

    if (!IsValidRange(remote_address, output.size()))
    {
        return Core::Result<std::size_t>::Failure("ReadMemory received an invalid address range");
    }

    const HANDLE process = Process::OpenProcessHandle(PROCESS_VM_READ, process_id);
    if (process == nullptr)
    {
        const DWORD error = GetLastError();
        return Core::Result<std::size_t>::Failure(
            "Unable to open process for ReadProcessMemory (error " + std::to_string(error) + ")");
    }

    SIZE_T transferred = 0;
    const BOOL succeeded = ReadProcessMemory(
        process,
        reinterpret_cast<LPCVOID>(remote_address),
        output.data(),
        output.size(),
        &transferred);
    const DWORD error = succeeded ? ERROR_SUCCESS : GetLastError();
    CloseHandle(process);

    if (transferred == output.size())
    {
        return Core::Result<std::size_t>::Success(static_cast<std::size_t>(transferred));
    }

    if (transferred == 0)
    {
        return Core::Result<std::size_t>::Failure(
            TransferMessage("ReadProcessMemory", 0, output.size(), error));
    }

    return Core::Result<std::size_t>::Warning(
        static_cast<std::size_t>(transferred),
        TransferMessage("ReadProcessMemory", transferred, output.size(), error));
}

Core::Result<std::size_t> Win32::WriteMemory(
    const std::uint32_t process_id,
    const std::uintptr_t remote_address,
    const std::span<const std::byte> input)
{
    if (input.empty())
    {
        return Core::Result<std::size_t>::Success(0);
    }

    if (!IsValidRange(remote_address, input.size()))
    {
        return Core::Result<std::size_t>::Failure("WriteMemory received an invalid address range");
    }

    const HANDLE process = Process::OpenProcessHandle(
        PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        process_id);
    if (process == nullptr)
    {
        const DWORD error = GetLastError();
        return Core::Result<std::size_t>::Failure(
            "Unable to open process for WriteProcessMemory (error " + std::to_string(error) + ")");
    }

    SIZE_T transferred = 0;
    const BOOL succeeded = WriteProcessMemory(
        process,
        reinterpret_cast<LPVOID>(remote_address),
        input.data(),
        input.size(),
        &transferred);
    DWORD error = succeeded ? ERROR_SUCCESS : GetLastError();
    BOOL cache_flushed = TRUE;
    if (succeeded && transferred > 0)
    {
        cache_flushed = FlushInstructionCache(
            process,
            reinterpret_cast<LPCVOID>(remote_address),
            transferred);
        if (!cache_flushed)
        {
            error = GetLastError();
        }
    }
    CloseHandle(process);

    if (transferred == input.size())
    {
        if (!cache_flushed)
        {
            return Core::Result<std::size_t>::Warning(
                static_cast<std::size_t>(transferred),
                TransferMessage("WriteProcessMemory / FlushInstructionCache", transferred, input.size(), error));
        }
        return Core::Result<std::size_t>::Success(static_cast<std::size_t>(transferred));
    }

    if (transferred == 0)
    {
        return Core::Result<std::size_t>::Failure(
            TransferMessage("WriteProcessMemory", 0, input.size(), error));
    }

    return Core::Result<std::size_t>::Warning(
        static_cast<std::size_t>(transferred),
        TransferMessage("WriteProcessMemory", transferred, input.size(), error));
}

}