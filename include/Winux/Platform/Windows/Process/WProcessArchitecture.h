#pragma once

#include <Winux/Contracts/IProcess.h>

#include <cstdint>

namespace Winux::Platform::Windows::Process::Detail {

Contracts::IProcess::Architecture ArchitectureFromMachine(std::uint16_t machine) noexcept;

}