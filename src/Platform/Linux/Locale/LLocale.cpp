#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Platform/General/Locale.h>

#include <cerrno>
#include <ctime>

namespace Winux::Platform::Linux {

namespace {

Core::Result<std::tm> ReadLocalTime(std::chrono::system_clock::time_point now)
{
    const auto whole_seconds = std::chrono::floor<std::chrono::seconds>(now);
    const auto timestamp = std::chrono::system_clock::to_time_t(whole_seconds);
    std::tm local_time{};
    if (localtime_r(&timestamp, &local_time) == nullptr)
    {
        return Core::Result<std::tm>::Failure(
            "Unable to determine local time (error " + std::to_string(errno) + ")");
    }

    return Core::Result<std::tm>::Success(local_time);
}

}

Contracts::ILocale& Linux::GetLocale()
{
    return *this;
}

Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>> Linux::GetTime()
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

Core::Result<std::chrono::year_month_day> Linux::GetDate()
{
    const auto local_time = ReadLocalTime(std::chrono::system_clock::now());
    if (local_time.Failed())
    {
        return Core::Result<std::chrono::year_month_day>::Failure(local_time.Message());
    }

    return Core::Result<std::chrono::year_month_day>::Success(
        MakeLocalDate(local_time.Value()));
}

Core::Result<std::chrono::seconds> Linux::GetTimezone()
{
    const auto local_time = ReadLocalTime(std::chrono::system_clock::now());
    if (local_time.Failed())
    {
        return Core::Result<std::chrono::seconds>::Failure(local_time.Message());
    }

    return Core::Result<std::chrono::seconds>::Success(
        std::chrono::seconds{local_time.Value().tm_gmtoff});
}

}