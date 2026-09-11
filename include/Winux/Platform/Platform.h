#pragma once

#include <Winux/Contracts/IPlatform.h>

#include <memory>

namespace Winux::Platform {

/*
    @summary
    Creates the platform implementation that matches the current operating system.
*/
std::unique_ptr<Contracts::IPlatform> create();

}
