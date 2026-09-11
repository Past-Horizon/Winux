#pragma once

#include <Winux/Core/Results.h>

namespace Winux::Contracts {

/*
    @summary
    Represents a named lock that can be acquired and released by one owner at a time.
*/
class IMutex {
public:
    virtual ~IMutex() = default;

    /*
        @summary
        Attempts to acquire the mutex without blocking.
    */
    virtual Core::Result<bool> try_acquire() = 0;

    /*
        @summary
        Releases the mutex if it is currently owned by this instance.
    */
    virtual Core::Result<void> release() = 0;

    /*
        @summary
        Returns whether this mutex instance currently owns the lock.
    */
    virtual bool owns_lock() const noexcept = 0;
};

}
