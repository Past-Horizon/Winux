#include <Winux/Platform/Windows/Win32.h>
#include <Winux/Platform/General/Locale.h>

#include <windows.h>

#include <cerrno>
#include <ctime>

namespace Winux::Platform::Windows {

namespace {

Core::Result<std::tm> ReadLocalTime(std::chrono::system_clock::time_point now)
{
    const auto whole_seconds = std::chrono::floor<std::chrono::seconds>(now);
    const auto timestamp = std::chrono::system_clock::to_time_t(whole_seconds);
    std::tm local_time{};
    if (localtime_s(&local_time, &timestamp) != 0)
    {
        return Core::Result<std::tm>::Failure(
            "Unable to determine local time (error " + std::to_string(errno) + ")");
    }

    return Core::Result<std::tm>::Success(local_time);
}

}

Contracts::ILocale& Win32::GetLocale()
{
    return *this;
}

Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>> Win32::GetTime()
{
    const auto now = std::chrono::system_clock::now();
    const auto local_time = ReadLocalTime(now);
    if (local_time.Failed())
    {
        return Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>>::Failure(
            local_time.Message());
    }

    const auto subseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - std::chrono::floor<std::chrono::seconds>(now));
    return Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>>::Success(
        MakeLocalTime(local_time.Value(), subseconds));
}

Core::Result<std::chrono::year_month_day> Win32::GetDate()
{
    const auto local_time = ReadLocalTime(std::chrono::system_clock::now());
    if (local_time.Failed())
    {
        return Core::Result<std::chrono::year_month_day>::Failure(local_time.Message());
    }

    return Core::Result<std::chrono::year_month_day>::Success(
        MakeLocalDate(local_time.Value()));
}

Core::Result<std::chrono::seconds> Win32::GetTimezone()
{
    TIME_ZONE_INFORMATION timezone{};
    const auto status = GetTimeZoneInformation(&timezone);
    if (status == TIME_ZONE_ID_INVALID)
    {
        return Core::Result<std::chrono::seconds>::Failure(
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

    return Core::Result<std::chrono::seconds>::Success(
        std::chrono::minutes{-bias});
}

}