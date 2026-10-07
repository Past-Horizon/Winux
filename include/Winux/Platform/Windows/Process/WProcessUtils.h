#pragma once

#include <Winux/Core/Results.h>

#include <chrono>
#include <cstdint>
#include <vector>

#include <windows.h>

namespace Winux::Platform::Windows::Process {

HANDLE OpenProcessHandle(DWORD desired_access, std::uint32_t process_id);

struct ProcessTreeEntry
{
	DWORD process_id;
	DWORD parent_process_id;
	ULONGLONG creation_time;
	bool has_creation_time;
};

Core::Result<std::vector<ProcessTreeEntry>> GetProcessTree();

bool IsDescendant(
	const std::vector<ProcessTreeEntry>& processes,
	DWORD process_id,
	DWORD possible_descendant);

DWORD TerminateNativeProcess(DWORD process_id, DWORD timeout);

bool TryGetTimeoutMilliseconds(
	std::chrono::milliseconds timeout,
	DWORD& timeout_milliseconds);

}