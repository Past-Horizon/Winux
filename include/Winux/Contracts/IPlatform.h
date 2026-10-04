#pragma once

#include <Winux/Core/Results.h>
#include <Winux/Contracts/Capabilities.h>
#include <Winux/Contracts/ICrypto.h>
#include <Winux/Contracts/IEnvironment.h>
#include <Winux/Contracts/IFileSystem.h>
#include <Winux/Contracts/INetwork.h>
#include <Winux/Contracts/ILocale.h>
#include <Winux/Contracts/IMutex.h>
#include <Winux/Contracts/IProcess.h>
#include <Winux/Contracts/ISystem.h>
#include <Winux/Contracts/ITerminal.h>

#include <memory>
#include <string>

namespace Winux::Contracts {

/*
    @summary
    Exposes the common and platform-specific services available on the current operating system.
*/
class IPlatform {
public:
    virtual ~IPlatform() = default;

    /*
        @summary
        Returns the process abstraction provided by the current platform.
    */
    virtual IProcess& GetProcess() = 0;

    /*
        @summary
        Returns the environment abstraction provided by the current platform.
    */
    virtual IEnvironment& GetEnvironment() = 0;

    /*
        @summary
        Returns the operating-system information interface provided by the current platform.

        @return
        The system service for the current platform.
    */
    virtual ISystem& GetSystem() = 0;

    /*
        @summary
        Returns the locale and local date/time interface provided by the current platform.
    */
    virtual ILocale& GetLocale() = 0;

    /*
        @summary
        Returns the file-system abstraction provided by the current platform.
    */
    virtual IFileSystem& GetFileSystem() = 0;

    /*
        @summary
        Returns the terminal abstraction provided by the current platform.
    */
    virtual ITerminal& GetTerminal() = 0;

    /*
        @summary
        Returns the operating-system-backed cryptographic random-byte service.
    */
    virtual ICrypto& GetCrypto() = 0;

    /*
        @summary
        Returns the HTTP network service.
    */
    virtual INetwork& GetNetwork() = 0;

    /*
        @summary
        Creates a named mutex for cross-thread or cross-process coordination.

        @param name
        Unique mutex name.
    */
    virtual Core::Result<std::unique_ptr<IMutex>> CreateMutex(
        const std::wstring& name) = 0;

    /*
        @summary
        Reports which process features are supported on this platform.
    */
    virtual CapabilitySet SupportedFeatures() const = 0;
};

} 
