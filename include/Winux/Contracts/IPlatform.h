#pragma once

#include <Winux/Contracts/IProcess.h>

namespace Winux::Contracts {

class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual IProcess& process() = 0;
};

} 
