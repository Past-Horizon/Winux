#include <Winux/Platform/Windows/Process/WProcessUtils.h>

#include <TlHelp32.h>

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace Winux::Platform::Windows::Process {

HANDLE OpenProcessHandle(const DWORD desired_access, const std::uint32_t process_id)
{
    return ::OpenProcess(desired_access, FALSE, process_id);
}

namespace {

std::optional<ULONGLONG> GetProcessCreationTime(const DWORD process_id)
{
    const HANDLE process = OpenProcessHandle(
        PROCESS_QUERY_LIMITED_INFORMATION,
        process_id);
    if (process == nullptr)
    {
        return std::nullopt;
    }

    FILETIME created{};
    FILETIME exited{};
    FILETIME kernel{};
    FILETIME user{};
    const BOOL succeeded = GetProcessTimes(process, &created, &exited, &kernel, &user);
    CloseHandle(process);
    if (!succeeded)
    {
        return std::nullopt;
    }

    return (static_cast<ULONGLONG>(created.dwHighDateTime) << 32) |
        created.dwLowDateTime;
}

}

Core::Result<std::vector<ProcessTreeEntry>> GetProcessTree()
{
    std::vector<ProcessTreeEntry> processes;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        const DWORD error = GetLastError();
        return Core::Result<std::vector<ProcessTreeEntry>>::Failure(
            "Unable to snapshot process tree (error " + std::to_string(error) + ")");
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot, &entry))
    {
        const DWORD error = GetLastError();
        CloseHandle(snapshot);
        if (error == ERROR_NO_MORE_FILES)
        {
            return Core::Result<std::vector<ProcessTreeEntry>>::Success({});
        }
        return Core::Result<std::vector<ProcessTreeEntry>>::Failure(
            "Unable to enumerate process tree (error " + std::to_string(error) + ")");
    }

    do
    {
        const auto creation_time = GetProcessCreationTime(entry.th32ProcessID);
        processes.push_back({
            entry.th32ProcessID,
            entry.th32ParentProcessID,
            creation_time.value_or(0),
            creation_time.has_value() });
    } while (Process32NextW(snapshot, &entry));
    const DWORD error = GetLastError();
    CloseHandle(snapshot);
    if (error != ERROR_NO_MORE_FILES)
    {
        return Core::Result<std::vector<ProcessTreeEntry>>::Failure(
            "Unable to enumerate process tree (error " + std::to_string(error) + ")");
    }

    return Core::Result<std::vector<ProcessTreeEntry>>::Success(std::move(processes));
}

bool IsDescendant(
    const std::vector<ProcessTreeEntry>& processes,
    const DWORD process_id,
    const DWORD possible_descendant)
{
    DWORD child_process_id = possible_descendant;
    std::unordered_set<DWORD> visited;
    for (std::size_t hops = 0; hops < processes.size(); ++hops)
    {
        if (!visited.insert(child_process_id).second)
        {
            return false;
        }

        const auto parent = std::find_if(
            processes.begin(),
            processes.end(),
            [child_process_id](const ProcessTreeEntry& entry)
            {
                return entry.process_id == child_process_id;
            });
        if (parent == processes.end() || !parent->has_creation_time ||
            parent->parent_process_id == 0)
        {
            return false;
        }

        const auto ancestor = std::find_if(
            processes.begin(),
            processes.end(),
            [parent_process_id = parent->parent_process_id](const ProcessTreeEntry& entry)
            {
                return entry.process_id == parent_process_id;
            });
        if (ancestor == processes.end() || !ancestor->has_creation_time ||
            ancestor->creation_time > parent->creation_time)
        {
            return false;
        }

        if (parent->parent_process_id == process_id)
        {
            return true;
        }
        child_process_id = parent->parent_process_id;
    }

    return false;
}

DWORD TerminateNativeProcess(const DWORD process_id, const DWORD timeout)
{
    const HANDLE process = OpenProcessHandle(
        PROCESS_TERMINATE | SYNCHRONIZE,
        process_id);
    if (process == nullptr)
    {
        return GetLastError();
    }

    DWORD error = ERROR_SUCCESS;
    if (::TerminateProcess(process, 1))
    {
        const DWORD wait_result = WaitForSingleObject(process, timeout);
        if (wait_result == WAIT_TIMEOUT)
        {
            error = ERROR_TIMEOUT;
        }
        else if (wait_result == WAIT_FAILED)
        {
            error = GetLastError();
        }
        else if (wait_result != WAIT_OBJECT_0)
        {
            error = ERROR_GEN_FAILURE;
        }
    }
    else
    {
        error = GetLastError();
    }

    CloseHandle(process);
    return error;
}

bool TryGetTimeoutMilliseconds(
    const std::chrono::milliseconds timeout,
    DWORD& timeout_milliseconds)
{
    if (timeout < std::chrono::milliseconds::zero() ||
        timeout.count() >= static_cast<std::chrono::milliseconds::rep>(INFINITE))
    {
        return false;
    }

    timeout_milliseconds = static_cast<DWORD>(timeout.count());
    return true;
}

}
