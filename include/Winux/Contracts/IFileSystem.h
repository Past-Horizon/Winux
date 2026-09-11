#pragma once

#include <Winux/Core/Results.h>

#include <filesystem>
#include <ios>
#include <string>
#include <string_view>

namespace Winux::Contracts {

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual Core::Result<std::filesystem::path> home() = 0;
    virtual Core::Result<std::filesystem::path> desktop() = 0;
    virtual Core::Result<std::filesystem::path> app_data() = 0;
    virtual Core::Result<std::filesystem::path> temp() = 0;
    virtual Core::Result<std::wstring> get_env(const std::wstring& name) = 0;
    virtual Core::Result<void> set_env(
        const std::wstring& name,
        const std::wstring& value) = 0;
    virtual Core::Result<void> unset_env(const std::wstring& name) = 0;
    virtual Core::Result<std::string> read_file(
        const std::filesystem::path& file,
        std::ios::openmode mode = std::ios::in | std::ios::binary) = 0;
    virtual Core::Result<void> write_file(
        const std::filesystem::path& file,
        std::string_view contents,
        std::ios::openmode mode = std::ios::out | std::ios::binary | std::ios::trunc) = 0;
};

}
