#include <Winux/Platform/Windows/Win32.h>
#include <Winux/Platform/General/Locale.h>

#include <windows.h>

#include <cerrno>
#include <ctime>

namespace Winux::Platform::Windows {

namespace {

Core::Result<std::tm> get_local_time(std::chrono::system_clock::time_point now)
{
    const auto whole_seconds = std::chrono::floor<std::chrono::seconds>(now);
    const auto timestamp = std::chrono::system_clock::to_time_t(whole_seconds);
    std::tm local_time{};
    if (localtime_s(&local_time, &timestamp) != 0)
    {
        return Core::Result<std::tm>::failure(
            "Unable to determine local time (error " + std::to_string(errno) + ")");
    }

    return Core::Result<std::tm>::success(local_time);
}

}

Contracts::ILocale& Win32::locale()
{
    return *this;
}

Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>> Win32::get_time()
{
    const auto now = std::chrono::system_clock::now();
    const auto local_time = get_local_time(now);
    if (local_time.failed())
    {
        return Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>>::failure(
            local_time.message());
    }

    const auto subseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - std::chrono::floor<std::chrono::seconds>(now));
    return Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>>::success(
        detail::make_local_time(local_time.value(), subseconds));
}

Core::Result<std::chrono::year_month_day> Win32::get_date()
{
    const auto local_time = get_local_time(std::chrono::system_clock::now());
    if (local_time.failed())
    {
        return Core::Result<std::chrono::year_month_day>::failure(local_time.message());
    }

    return Core::Result<std::chrono::year_month_day>::success(
        detail::make_local_date(local_time.value()));
}

Core::Result<std::chrono::seconds> Win32::get_timezone()
{
    TIME_ZONE_INFORMATION timezone{};
    const auto status = GetTimeZoneInformation(&timezone);
    if (status == TIME_ZONE_ID_INVALID)
    {
        return Core::Result<std::chrono::seconds>::failure(
            "Unable to determine the local timezone (error " + std::to_string(GetLastError()) + ")");
    }

    auto bias = timezone.Bias;
    if (status == TIME_ZONE_ID_DAYLIGHT)
    {
        bias += timezone.DaylightBias;
    }
    else if (status == TIME_ZONE_ID_STANDARD)
    {
        bias += timezone.StandardBias;
    }

    return Core::Result<std::chrono::seconds>::success(
        std::chrono::minutes{-bias});
}

}