#include <Winux/Platform/Linux/Linux.h>
#include <Winux/Platform/General/Locale.h>
#include <Winux/Utils/Strings.h>

#include <cerrno>
#include <ctime>

namespace Winux::Platform::Linux {

namespace {

Core::Result<std::tm> get_local_time(std::chrono::system_clock::time_point now)
{
    const auto whole_seconds = std::chrono::floor<std::chrono::seconds>(now);
    const auto timestamp = std::chrono::system_clock::to_time_t(whole_seconds);
    std::tm local_time{};
    if (localtime_r(&timestamp, &local_time) == nullptr)
    {
        return Core::Result<std::tm>::failure(
            "Unable to determine local time (error " + std::to_string(errno) + ")");
    }

    return Core::Result<std::tm>::success(local_time);
}

}

Contracts::ILocale& Linux::locale()
{
    return *this;
}

Core::Result<std::chrono::hh_mm_ss<std::chrono::milliseconds>> Linux::get_time()
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

Core::Result<std::chrono::year_month_day> Linux::get_date()
{
    const auto local_time = get_local_time(std::chrono::system_clock::now());
    if (local_time.failed())
    {
        return Core::Result<std::chrono::year_month_day>::failure(local_time.message());
    }

    return Core::Result<std::chrono::year_month_day>::success(
        detail::make_local_date(local_time.value()));
}

Core::Result<std::chrono::seconds> Linux::get_timezone()
{
    const auto local_time = get_local_time(std::chrono::system_clock::now());
    if (local_time.failed())
    {
        return Core::Result<std::chrono::seconds>::failure(local_time.message());
    }

    return Core::Result<std::chrono::seconds>::success(
        std::chrono::seconds{local_time.value().tm_gmtoff});
}

std::wstring Linux::Utf8ToWide(const std::string& input)
{
    return String::Utf8ToWide(input);
}

std::string Linux::WideToUtf8(const std::wstring& input)
{
    return String::WideToUtf8(input);
}

}