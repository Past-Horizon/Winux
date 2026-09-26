#include <Winux/Platform/General/Locale.h>

namespace Winux::Platform::detail {

std::chrono::hh_mm_ss<std::chrono::milliseconds> make_local_time(
    const std::tm& local_time,
    std::chrono::milliseconds subseconds)
{
    const auto since_midnight = std::chrono::hours{local_time.tm_hour}
        + std::chrono::minutes{local_time.tm_min}
        + std::chrono::seconds{local_time.tm_sec}
        + subseconds;
    return std::chrono::hh_mm_ss<std::chrono::milliseconds>{since_midnight};
}

std::chrono::year_month_day make_local_date(const std::tm& local_time)
{
    return std::chrono::year{local_time.tm_year + 1900}
        / std::chrono::month{static_cast<unsigned>(local_time.tm_mon + 1)}
        / std::chrono::day{static_cast<unsigned>(local_time.tm_mday)};
}

}