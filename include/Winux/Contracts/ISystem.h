#pragma once

#include <Winux/Core/Results.h>

#include <string>

namespace Winux::Contracts {

/*
    @summary
    Exposes operating-system identity information.
*/
class ISystem {
public:
    virtual ~ISystem() = default;

    /*
        @summary
        Returns the username of the current user.
    */
    virtual Core::Result<std::wstring> get_username() = 0;
};

}
