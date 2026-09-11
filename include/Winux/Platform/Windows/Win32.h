#pragma once

#include <Winux/Contracts/IPlatform.h>
#include <iostream>

namespace Winux::Platform::Windows {

class Win32 final : public Contracts::IPlatform, public Contracts::IProcess, public Contracts::IFileSystem {
public:
    ~Win32() override = default;

    Contracts::IProcess& process() override;
    Contracts::IFileSystem& file_system() override;
    Contracts::IProcess::ProcessOptions supported_features() const override;
    Core::Result<std::filesystem::path> home() override;
    Core::Result<std::filesystem::path> desktop() override;
    Core::Result<std::filesystem::path> app_data() override;
    Core::Result<std::filesystem::path> temp() override;
    Core::Result<std::wstring> get_env(const std::wstring& name) override;
    Core::Result<void> set_env(
        const std::wstring& name,
        const std::wstring& value) override;
    Core::Result<void> unset_env(const std::wstring& name) override;
    Core::Result<std::string> read_file(
        const std::filesystem::path& file,
        std::ios::openmode mode) override;
    Core::Result<void> write_file(
        const std::filesystem::path& file,
        std::string_view contents,
        std::ios::openmode mode) override;
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
