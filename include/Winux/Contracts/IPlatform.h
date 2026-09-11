#pragma once

#include <Winux/Core/Results.h>
#include <Winux/Contracts/IProcess.h>

namespace Winux::Contracts {

class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual IProcess& process() = 0;
    virtual IProcess::ProcessOptions supported_features() const = 0;
};

} 
