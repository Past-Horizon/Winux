#pragma once

#include <Winux/Core/Results.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace Winux::Contracts {

/*
    @summary
    Reads and writes memory in a process address space.
*/
class IMemory {
public:
    virtual ~IMemory() = default;

    /*
        @summary
        Reads memory from a process into the supplied buffer.

        @param process_id
        Identifier of the process to read.

        @param remote_address
        Address in the target process.

        @param output
        Destination buffer. Empty buffers succeed without validating the process.

        @returns
        The number of bytes transferred. A short nonzero transfer is returned as a warning.
    */
    virtual Core::Result<std::size_t> ReadMemory(
        std::uint32_t process_id,
        std::uintptr_t remote_address,
        std::span<std::byte> output) = 0;

    /*
        @summary
        Writes memory in a process from the supplied buffer.

        @param process_id
        Identifier of the process to write.

        @param remote_address
        Address in the target process.

        @param input
        Source buffer. Empty buffers succeed without validating the process.

        On Linux, access is subject to ptrace permissions, including Yama policy.

        @returns
        The number of bytes transferred. A short nonzero transfer is returned as a warning.
    */
    virtual Core::Result<std::size_t> WriteMemory(
        std::uint32_t process_id,
        std::uintptr_t remote_address,
        std::span<const std::byte> input) = 0;
};

}