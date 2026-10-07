#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Platform/Linux/Process/LProcessArchitecture.h>
#include <Winux/Platform/Linux/Process/LProcessUtils.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <elf.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace Winux::Platform::Linux::Process::Detail {

Contracts::IProcess::Architecture ArchitectureFromElfMachine(
    const std::uint16_t machine,
    const unsigned char elf_class,
    const unsigned char elf_data) noexcept
{
    using Architecture = Contracts::IProcess::Architecture;

    if (elf_data != ELFDATA2LSB)
    {
        return Architecture::Unknown;
    }

    if (machine == EM_386 && elf_class == ELFCLASS32)
    {
        return Architecture::X86;
    }
    if (machine == EM_X86_64 && elf_class == ELFCLASS64)
    {
        return Architecture::X64;
    }
    if (machine == EM_ARM && elf_class == ELFCLASS32)
    {
        return Architecture::Arm;
    }
    if (machine == EM_AARCH64 && elf_class == ELFCLASS64)
    {
        return Architecture::Arm64;
    }

    return Architecture::Unknown;
}

}

namespace Winux::Platform::Linux {

Core::Result<Contracts::IProcess::Architecture> Linux::GetArchitecture(
    const std::uint32_t process_id)
{
    using Architecture = Contracts::IProcess::Architecture;

    if (!Process::Detail::IsValidProcessId(process_id))
    {
        return Core::Result<Architecture>::Failure(
            "Unable to determine process architecture: invalid process ID");
    }

    const std::filesystem::path executable_path = std::filesystem::path("/proc") /
        std::to_string(process_id) / "exe";
    std::ifstream executable(executable_path, std::ios::binary);
    if (!executable)
    {
        return Core::Result<Architecture>::Failure(
            "Unable to open process executable to determine architecture");
    }

    std::array<unsigned char, EI_NIDENT> identification{};
    executable.read(
        reinterpret_cast<char*>(identification.data()),
        static_cast<std::streamsize>(identification.size()));
    if (executable.gcount() != static_cast<std::streamsize>(identification.size()) ||
        std::memcmp(identification.data(), ELFMAG, SELFMAG) != 0 ||
        identification[EI_VERSION] != EV_CURRENT)
    {
        return Core::Result<Architecture>::Failure(
            "Process executable has an invalid ELF identification");
    }

    if ((identification[EI_CLASS] != ELFCLASS32 &&
         identification[EI_CLASS] != ELFCLASS64) ||
        (identification[EI_DATA] != ELFDATA2LSB &&
         identification[EI_DATA] != ELFDATA2MSB))
    {
        return Core::Result<Architecture>::Failure(
            "Process executable has an unsupported ELF format");
    }

    executable.seekg(static_cast<std::streamoff>(offsetof(Elf64_Ehdr, e_machine)));
    std::array<unsigned char, sizeof(std::uint16_t)> machine_bytes{};
    executable.read(
        reinterpret_cast<char*>(machine_bytes.data()),
        static_cast<std::streamsize>(machine_bytes.size()));
    if (executable.gcount() != static_cast<std::streamsize>(machine_bytes.size()))
    {
        return Core::Result<Architecture>::Failure(
            "Unable to read process executable architecture");
    }

    const std::uint16_t machine = identification[EI_DATA] == ELFDATA2LSB
        ? static_cast<std::uint16_t>(machine_bytes[0] | (machine_bytes[1] << 8))
        : static_cast<std::uint16_t>((machine_bytes[0] << 8) | machine_bytes[1]);

    return Core::Result<Architecture>::Success(
        Process::Detail::ArchitectureFromElfMachine(
            machine,
            identification[EI_CLASS],
            identification[EI_DATA]));
}

}