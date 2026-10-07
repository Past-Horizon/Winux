#pragma once

#include <Winux/Core/Results.h>

#include <cstdint>
#include <optional>
#include <sys/types.h>
#include <vector>

namespace Winux::Platform::Linux::Process::Detail {

struct ProcessInfo
{
    char state;
    pid_t parent_process_id;
};

bool IsValidProcessId(std::uint32_t process_id) noexcept;
std::optional<ProcessInfo> ReadProcessInfo(pid_t process_id);
bool IsZombieProcess(pid_t process_id);
Core::Result<std::vector<pid_t>> GetProcessDescendants(pid_t root_process_id);

}