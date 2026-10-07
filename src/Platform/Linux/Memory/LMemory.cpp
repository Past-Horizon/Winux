#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Platform/Linux/Process/LProcessUtils.h>

#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <string>

namespace Winux::Platform::Linux {

namespace {

bool IsValidRange(const std::uintptr_t address, const std::size_t size)
{
    return address != 0 &&
        static_cast<std::uintptr_t>(size - 1) <=
            std::numeric_limits<std::uintptr_t>::max() - address;
}

std::string TransferMessage(
    const char* operation,
    const std::size_t transferred,
    const std::size_t requested)
{
    return std::string(operation) + " transferred " + std::to_string(transferred) +
        " of " + std::to_string(requested) + " bytes";
}

}

Contracts::IMemory& Linux::GetMemory()
{
    return *this;
}

Core::Result<std::size_t> Linux::ReadMemory(
    const std::uint32_t process_id,
    const std::uintptr_t remote_address,
    const std::span<std::byte> output)
{
    if (output.empty())
    {
        return Core::Result<std::size_t>::Success(0);
    }

    if (!Process::Detail::IsValidProcessId(process_id))
    {
        return Core::Result<std::size_t>::Failure("ReadMemory received an invalid process ID");
    }
    if (!IsValidRange(remote_address, output.size()))
    {
        return Core::Result<std::size_t>::Failure("ReadMemory received an invalid address range");
    }
    if (output.size() > static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()))
    {
        return Core::Result<std::size_t>::Failure("ReadMemory buffer exceeds the maximum transfer size");
    }

    iovec local_iov{ output.data(), output.size() };
    iovec remote_iov{ reinterpret_cast<void*>(remote_address), output.size() };
    const ssize_t transferred = process_vm_readv(
        static_cast<pid_t>(process_id),
        &local_iov,
        1,
        &remote_iov,
        1,
        0);
    if (transferred < 0)
    {
        const int error = errno;
        return Core::Result<std::size_t>::Failure(
            "process_vm_readv failed (error " + std::to_string(error) + ")");
    }

    const auto byte_count = static_cast<std::size_t>(transferred);
    if (byte_count == output.size())
    {
        return Core::Result<std::size_t>::Success(byte_count);
    }
    if (byte_count == 0)
    {
        return Core::Result<std::size_t>::Failure(TransferMessage("process_vm_readv", 0, output.size()));
    }

    return Core::Result<std::size_t>::Warning(
        byte_count,
        TransferMessage("process_vm_readv", byte_count, output.size()));
}

Core::Result<std::size_t> Linux::WriteMemory(
    const std::uint32_t process_id,
    const std::uintptr_t remote_address,
    const std::span<const std::byte> input)
{
    if (input.empty())
    {
        return Core::Result<std::size_t>::Success(0);
    }

    if (!Process::Detail::IsValidProcessId(process_id))
    {
        return Core::Result<std::size_t>::Failure("WriteMemory received an invalid process ID");
    }
    if (!IsValidRange(remote_address, input.size()))
    {
        return Core::Result<std::size_t>::Failure("WriteMemory received an invalid address range");
    }
    if (input.size() > static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()))
    {
        return Core::Result<std::size_t>::Failure("WriteMemory buffer exceeds the maximum transfer size");
    }

    iovec local_iov{ const_cast<std::byte*>(input.data()), input.size() };
    iovec remote_iov{ reinterpret_cast<void*>(remote_address), input.size() };
    ssize_t transferred = process_vm_writev(
        static_cast<pid_t>(process_id),
        &local_iov,
        1,
        &remote_iov,
        1,
        0);
    const char* operation = "process_vm_writev";
    if (transferred < 0)
    {
        const int error = errno;
        if (error != EFAULT)
        {
            return Core::Result<std::size_t>::Failure(
                "process_vm_writev failed (error " + std::to_string(error) + ")");
        }

        const auto maximum_offset = static_cast<std::uintptr_t>(
            (std::numeric_limits<off_t>::max)());
        if (remote_address > maximum_offset || input.size() - 1 > maximum_offset - remote_address)
        {
            return Core::Result<std::size_t>::Failure(
                "WriteMemory address exceeds the /proc memory offset range");
        }

        const std::string memory_path =
            "/proc/" + std::to_string(process_id) + "/mem";
        const int memory_file = open(memory_path.c_str(), O_WRONLY | O_CLOEXEC);
        if (memory_file == -1)
        {
            const int open_error = errno;
            return Core::Result<std::size_t>::Failure(
                "Unable to open process memory (error " + std::to_string(open_error) + ")");
        }

        do
        {
            transferred = pwrite(
                memory_file,
                input.data(),
                input.size(),
                static_cast<off_t>(remote_address));
        } while (transferred == -1 && errno == EINTR);
        const int write_error = transferred < 0 ? errno : 0;
        close(memory_file);
        if (transferred < 0)
        {
            return Core::Result<std::size_t>::Failure(
                "/proc process memory write failed (error " +
                std::to_string(write_error) + ")");
        }
        operation = "/proc process memory write";
    }

    const auto byte_count = static_cast<std::size_t>(transferred);
    if (byte_count == input.size())
    {
        return Core::Result<std::size_t>::Success(byte_count);
    }
    if (byte_count == 0)
    {
        return Core::Result<std::size_t>::Failure(TransferMessage(operation, 0, input.size()));
    }

    return Core::Result<std::size_t>::Warning(
        byte_count,
        TransferMessage(operation, byte_count, input.size()));
}

}