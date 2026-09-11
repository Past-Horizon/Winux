#pragma once

#include <Winux/Core/Results.h>

#include <filesystem>
#include <string>

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
};

}
