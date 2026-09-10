#pragma once

#include <Winux/Contracts/IPlatform.h>

#include <memory>

namespace Winux::Platform {

std::unique_ptr<Contracts::IPlatform> create();

}
