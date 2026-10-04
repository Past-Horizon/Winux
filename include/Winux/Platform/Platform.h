#pragma once

#ifdef _WIN32
#include <Winux/Platform/Windows/WindowsMacroCleanup.h>
#endif

#include <Winux/Contracts/IPlatform.h>

#include <memory>

namespace Winux::Platform {

/*
    @summary
    Initializes the platform implementation that matches the current operating system.

    @returns
    A failure result if the platform has already been initialized.
*/
Core::Result<void> Initialize();

/*
    @summary
    Reports whether the platform implementation has been initialized.
*/
Core::Result<bool> IsInitialized();

/*
    @summary
    Shuts down the platform implementation and releases its services.

    @warning
    All service references become invalid. Do not call while services are in use.
*/
Core::Result<void> Shutdown();

}
