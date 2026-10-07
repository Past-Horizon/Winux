#pragma once

#include <Winux/Contracts/IProcess.h>

#include <cstdint>

namespace Winux::Platform::Linux::Process::Detail {

Contracts::IProcess::Architecture ArchitectureFromElfMachine(
    std::uint16_t machine,
    unsigned char elf_class,
    unsigned char elf_data) noexcept;

}