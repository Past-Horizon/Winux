#pragma once

#include <Winux/Winux.h>

namespace Winux::Testing {

inline void InitializePlatformOnce()
{
    static const bool initialized = []
    {
        Platform::Initialize();
        return true;
    }();
    (void)initialized;
}

template <typename Service>
Service& Get()
{
    InitializePlatformOnce();
    return Winux::Get<Service>();
}

}