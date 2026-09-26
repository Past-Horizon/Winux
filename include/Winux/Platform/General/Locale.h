#pragma once

#include <chrono>
#include <ctime>

namespace Winux::Platform::detail {

/*
    @summary
    Converts local calendar fields to a structured 24-hour time value.
*/
std::chrono::hh_mm_ss<std::chrono::milliseconds> make_local_time(
    const std::tm& local_time,
    std::chrono::milliseconds subseconds);

/*
    @summary
    Converts local calendar fields to a structured civil date.
*/
std::chrono::year_month_day make_local_date(const std::tm& local_time);

}