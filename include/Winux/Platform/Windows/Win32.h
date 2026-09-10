#pragma once

#include <Winux/Contracts/IPlatform.h>
#include <iostream>

namespace Winux::Platform::Windows {

class Win32 final : public Contracts::IPlatform, public Contracts::IProcess {
public:
    ~Win32() override = default;

    Contracts::IProcess& process() override;
    std::vector<std::uint32_t> find_processes(const std::wstring& name) override;
    std::uint32_t find_process(const std::wstring& name) override;
    std::uint32_t create_process(const std::wstring& application) override;
    bool terminate_process(std::uint32_t process_id) override;
};

}
