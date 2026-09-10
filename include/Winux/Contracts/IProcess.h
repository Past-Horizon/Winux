#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

namespace Winux::Contracts {

class IProcess {
public:
    virtual ~IProcess() = default;

    virtual std::vector<std::uint32_t> find_processes(const std::wstring& name) = 0;
    virtual std::uint32_t find_process(const std::wstring& name) = 0;
    virtual std::uint32_t create_process(const std::wstring& application) = 0;
    virtual bool terminate_process(std::uint32_t process_id) = 0;
};

} // namespace Winux::Contracts
