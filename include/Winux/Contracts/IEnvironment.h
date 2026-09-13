#pragma once

#include <Winux/Core/Results.h>

#include <string>

namespace Winux::Contracts {

/*
    @summary
    Exposes environment-variable access for the current platform.
*/
class IEnvironment {
public:
    virtual ~IEnvironment() = default;

    /*
        @summary
        Reads the value of an environment variable.

        @param name
        Environment variable name.
    */
    virtual Core::Result<std::wstring> get_env(const std::wstring& name) = 0;

    /*
        @summary
        Sets an environment variable to a new value.

        @param name
        Environment variable name.

        @param value
        Value to set.
    */
    virtual Core::Result<void> set_env(
        const std::wstring& name,
        const std::wstring& value) = 0;

    /*
        @summary
        Removes an environment variable.

        @param name
        Environment variable name.
    */
    virtual Core::Result<void> unset_env(const std::wstring& name) = 0;
};

}
