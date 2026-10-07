#include <Winux/Platform/Linux/Process/LProcessUtils.h>

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace Winux::Platform::Linux::Process::Detail {

bool IsValidProcessId(const std::uint32_t process_id) noexcept
{
    return process_id != 0 &&
        static_cast<std::uint64_t>(process_id) <=
            static_cast<std::uint64_t>((std::numeric_limits<pid_t>::max)());
}

std::optional<ProcessInfo> ReadProcessInfo(const pid_t process_id)
{
    std::ifstream stat_file(
        std::filesystem::path("/proc") / std::to_string(process_id) / "stat");
    std::string stat_line;
    if (!stat_file || !std::getline(stat_file, stat_line))
    {
        return std::nullopt;
    }

    const std::size_t command_end = stat_line.rfind(')');
    if (command_end == std::string::npos || command_end + 2 >= stat_line.size())
    {
        return std::nullopt;
    }

    std::istringstream fields(stat_line.substr(command_end + 2));
    ProcessInfo info{};
    if (!(fields >> info.state >> info.parent_process_id))
    {
        return std::nullopt;
    }

    return info;
}

bool IsZombieProcess(const pid_t process_id)
{
    const auto info = ReadProcessInfo(process_id);
    return info.has_value() && info->state == 'Z';
}

Core::Result<std::vector<pid_t>> GetProcessDescendants(const pid_t root_process_id)
{
    std::unordered_map<pid_t, std::vector<pid_t>> children_by_parent;
    std::error_code error;
    std::filesystem::directory_iterator entry("/proc", error);
    if (error)
    {
        return Core::Result<std::vector<pid_t>>::Failure(
            "Unable to enumerate process tree (error " + error.message() + ")");
    }

    const std::filesystem::directory_iterator end;
    while (entry != end)
    {
        const std::string name = entry->path().filename().string();
        std::uint32_t process_id = 0;
        const auto [parsed_end, parse_error] = std::from_chars(
            name.data(),
            name.data() + name.size(),
            process_id);
        if (parse_error == std::errc{} && parsed_end == name.data() + name.size() &&
            IsValidProcessId(process_id))
        {
            const auto info = ReadProcessInfo(static_cast<pid_t>(process_id));
            if (info.has_value())
            {
                children_by_parent[info->parent_process_id].push_back(
                    static_cast<pid_t>(process_id));
            }
        }

        entry.increment(error);
        if (error)
        {
            return Core::Result<std::vector<pid_t>>::Failure(
                "Unable to enumerate process tree (error " + error.message() + ")");
        }
    }

    std::vector<pid_t> descendants;
    std::vector<pid_t> pending{ root_process_id };
    std::unordered_set<pid_t> visited{ root_process_id };
    while (!pending.empty())
    {
        const pid_t parent = pending.back();
        pending.pop_back();
        const auto children = children_by_parent.find(parent);
        if (children == children_by_parent.end())
        {
            continue;
        }

        for (const pid_t child : children->second)
        {
            if (visited.insert(child).second)
            {
                descendants.push_back(child);
                pending.push_back(child);
            }
        }
    }

    std::reverse(descendants.begin(), descendants.end());
    return Core::Result<std::vector<pid_t>>::Success(std::move(descendants));
}

}