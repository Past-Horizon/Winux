#pragma once

#include <Winux/Core/Results.h>
#include <Winux/Contracts/IFileSystem.h>
#include <Winux/Contracts/IMutex.h>
#include <Winux/Contracts/IProcess.h>

#include <memory>
#include <string>

namespace Winux::Contracts {

class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual IProcess& process() = 0;
    virtual IFileSystem& file_system() = 0;
    virtual Core::Result<std::unique_ptr<IMutex>> create_mutex(
        const std::wstring& name) = 0;
    virtual IProcess::ProcessOptions supported_features() const = 0;
};

} 
