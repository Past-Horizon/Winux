#pragma once

#include <Winux/Core/Results.h>

namespace Winux::Contracts {

class IMutex {
public:
    virtual ~IMutex() = default;

    virtual Core::Result<bool> try_acquire() = 0;
    virtual Core::Result<void> release() = 0;
    virtual bool owns_lock() const noexcept = 0;
};

}
