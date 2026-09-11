#pragma once

#include <Winux/Contracts/IPlatform.h>
#include <iostream>

namespace Winux::Platform::Windows {

class Win32 final : public Contracts::IPlatform, public Contracts::IProcess {
public:
    ~Win32() override = default;

    Contracts::IProcess& process() override;
    Contracts::IProcess::ProcessOptions supported_features() const override;
    Core::Result<std::vector<std::uint32_t>> find_processes(const std::wstring& name) override;
    Core::Result<std::optional<std::uint32_t>> find_process(const std::wstring& name) override;
    Core::Result<std::filesystem::path> find_location(std::uint32_t process_id) override;
    Core::Result<bool> is_running(std::uint32_t process_id) override;
    Core::Result<std::uint32_t> create_process_impl(
        const std::wstring& application,
        Contracts::IProcess::ProcessOptions requested_features) override;
    Core::Result<void> terminate_process(std::uint32_t process_id) override;
};

}
