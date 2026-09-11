#pragma once

#include <Winux/Core/Results.h>
#include <Winux/Contracts/IFileSystem.h>
#include <Winux/Contracts/IMutex.h>
#include <Winux/Contracts/IProcess.h>

#include <memory>
#include <string>

namespace Winux::Contracts {

/*
    @summary
    Exposes the platform-specific process, file-system, and mutex services.
*/
class IPlatform {
public:
    virtual ~IPlatform() = default;

    /*
        @summary
        Returns the process abstraction provided by the current platform.
    */
    virtual IProcess& process() = 0;

    /*
        @summary
        Returns the file-system abstraction provided by the current platform.
    */
    virtual IFileSystem& file_system() = 0;

    /*
        @summary
        Creates a named mutex for cross-thread or cross-process coordination.

        @param name
        Unique mutex name.
    */
    virtual Core::Result<std::unique_ptr<IMutex>> create_mutex(
        const std::wstring& name) = 0;

    /*
        @summary
        Reports which process features are supported on this platform.
    */
    virtual IProcess::ProcessOptions supported_features() const = 0;
};

} 
