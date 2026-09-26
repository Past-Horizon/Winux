#pragma once

#include <Winux/Core/Results.h>

#include <chrono>

namespace Winux::Contracts {

/*
    @summary
    Exposes the current local date, time, and UTC offset as structured values.
*/
class ILocale {
public:
    virtual ~ILocale() = default;

    /*
        @summary
        Returns the current local wall-clock time with millisecond precision.
    */
    virtual Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>> get_time() = 0;

    /*
        @summary
        Returns the current local civil date.
    */
    virtual Core::Result<std::chrono::year_month_day> get_date() = 0;

    /*
        @summary
        Returns the current local UTC offset, positive east of UTC.
    */
    virtual Core::Result<std::chrono::seconds> get_timezone() = 0;
};

}