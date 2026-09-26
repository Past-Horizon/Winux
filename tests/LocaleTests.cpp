#include <Winux/Winux.h>
#include <Winux/Platform/General/Locale.h>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <ctime>
#endif

namespace {

class LocaleTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        platform = Winux::Platform::create();
        ASSERT_NE(platform, nullptr);
        locale = &platform->locale();
    }

    std::unique_ptr<Winux::Contracts::IPlatform> platform;
    Winux::Contracts::ILocale* locale = nullptr;
};

TEST_F(LocaleTests, ReturnsStructuredLocalTimeDateAndTimezone)
{
    ASSERT_NE(locale, nullptr);

    const auto time = locale->get_time();
    ASSERT_TRUE(time.succeeded()) << time.message();
    EXPECT_GE(time.value().hours().count(), 0);
    EXPECT_LT(time.value().hours().count(), 24);
    EXPECT_GE(time.value().minutes().count(), 0);
    EXPECT_LT(time.value().minutes().count(), 60);
    EXPECT_GE(time.value().seconds().count(), 0);
    EXPECT_LT(time.value().seconds().count(), 60);
    EXPECT_GE(time.value().subseconds().count(), 0);
    EXPECT_LT(time.value().subseconds().count(), 1000);

    const auto date = locale->get_date();
    ASSERT_TRUE(date.succeeded()) << date.message();
    EXPECT_TRUE(date.value().ok());

    const auto timezone = locale->get_timezone();
    ASSERT_TRUE(timezone.succeeded()) << timezone.message();
    EXPECT_GT(timezone.value(), -std::chrono::hours{24});
    EXPECT_LT(timezone.value(), std::chrono::hours{24});

#ifdef _WIN32
    TIME_ZONE_INFORMATION timezone_info{};
    const auto timezone_status = GetTimeZoneInformation(&timezone_info);
    ASSERT_NE(timezone_status, TIME_ZONE_ID_INVALID);
    auto bias = timezone_info.Bias;
    if (timezone_status == TIME_ZONE_ID_DAYLIGHT)
    {
        bias += timezone_info.DaylightBias;
    }
    else if (timezone_status == TIME_ZONE_ID_STANDARD)
    {
        bias += timezone_info.StandardBias;
    }
    const auto expected_timezone = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::minutes{-bias});
#else
    const auto timestamp = std::time(nullptr);
    std::tm local_time{};
    ASSERT_NE(localtime_r(&timestamp, &local_time), nullptr);
    const auto expected_timezone = std::chrono::seconds{local_time.tm_gmtoff};
#endif
    EXPECT_EQ(timezone.value(), expected_timezone);
}

TEST(LocaleUtilityTests, ConvertsLocalTimeFieldsWithoutFormatting)
{
    std::tm local_time{};
    local_time.tm_hour = 23;
    local_time.tm_min = 58;
    local_time.tm_sec = 57;

    const auto time = Winux::Platform::detail::make_local_time(
        local_time, std::chrono::milliseconds{321});

    EXPECT_EQ(time.hours(), std::chrono::hours{23});
    EXPECT_EQ(time.minutes(), std::chrono::minutes{58});
    EXPECT_EQ(time.seconds(), std::chrono::seconds{57});
    EXPECT_EQ(time.subseconds(), std::chrono::milliseconds{321});
}

TEST(LocaleUtilityTests, ConvertsLeapDayToCivilDateFields)
{
    std::tm local_time{};
    local_time.tm_year = 124;
    local_time.tm_mon = 1;
    local_time.tm_mday = 29;

    const auto date = Winux::Platform::detail::make_local_date(local_time);

    ASSERT_TRUE(date.ok());
    EXPECT_EQ(date.year(), std::chrono::year{2024});
    EXPECT_EQ(date.month(), std::chrono::February);
    EXPECT_EQ(date.day(), std::chrono::day{29});
}

}