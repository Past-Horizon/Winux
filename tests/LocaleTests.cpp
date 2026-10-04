#include <Winux/Winux.h>
#include <Winux/Platform/General/Locale.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>

#include <chrono>

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
        Winux::Testing::InitializePlatformOnce();
        locale = &Winux::Get<Winux::Locale>();
    }

    Winux::Locale* locale = nullptr;
};

TEST_F(LocaleTests, ReturnsStructuredLocalTimeDateAndTimezone)
{
    ASSERT_NE(locale, nullptr);

    const auto time = locale->GetTime();
    ASSERT_TRUE(time.Succeeded()) << time.Message();
    EXPECT_GE(time.Value().hours().count(), 0);
    EXPECT_LT(time.Value().hours().count(), 24);
    EXPECT_GE(time.Value().minutes().count(), 0);
    EXPECT_LT(time.Value().minutes().count(), 60);
    EXPECT_GE(time.Value().seconds().count(), 0);
    EXPECT_LT(time.Value().seconds().count(), 60);
    EXPECT_GE(time.Value().subseconds().count(), 0);
    EXPECT_LT(time.Value().subseconds().count(), 1000);

    const auto date = locale->GetDate();
    ASSERT_TRUE(date.Succeeded()) << date.Message();
    EXPECT_TRUE(date.Value().ok());

    const auto timezone = locale->GetTimezone();
    ASSERT_TRUE(timezone.Succeeded()) << timezone.Message();
    EXPECT_GT(timezone.Value(), -std::chrono::hours{24});
    EXPECT_LT(timezone.Value(), std::chrono::hours{24});

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
    EXPECT_EQ(timezone.Value(), expected_timezone);
}

TEST(LocaleUtilityTests, ConvertsLocalTimeFieldsWithoutFormatting)
{
    std::tm local_time{};
    local_time.tm_hour = 23;
    local_time.tm_min = 58;
    local_time.tm_sec = 57;

    const auto time = Winux::Platform::MakeLocalTime(
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

    const auto date = Winux::Platform::MakeLocalDate(local_time);

    ASSERT_TRUE(date.ok());
    EXPECT_EQ(date.year(), std::chrono::year{2024});
    EXPECT_EQ(date.month(), std::chrono::February);
    EXPECT_EQ(date.day(), std::chrono::day{29});
}

TEST_F(LocaleTests, ConvertsUtf8AndWideTextStrictly)
{
    ASSERT_NE(locale, nullptr);

    const std::string utf8 = "\xF0\x9F\x98\x80";
    const auto wide = locale->Utf8ToWide(utf8);
    ASSERT_FALSE(wide.empty());
    EXPECT_EQ(locale->WideToUtf8(wide), utf8);

    EXPECT_TRUE(locale->Utf8ToWide("\xC0\xAF").empty());
    EXPECT_TRUE(locale->WideToUtf8(std::wstring(1, static_cast<wchar_t>(0xD800))).empty());
}

TEST_F(LocaleTests, ExposesUnicodeConversionsThroughLocaleContract)
{
    ASSERT_NE(locale, nullptr);

    const std::u32string scalars = U"A\U0001F642";
    const auto utf8 = locale->EncodeUtf8(scalars);
    ASSERT_TRUE(utf8.Succeeded()) << utf8.Message();

    const auto utf16 = locale->ToUtf16(utf8.Value());
    ASSERT_TRUE(utf16.Succeeded()) << utf16.Message();
    const auto decoded = locale->DecodeUtf16(utf16.Value());
    ASSERT_TRUE(decoded.Succeeded()) << decoded.Message();
    EXPECT_EQ(decoded.Value(), scalars);
    EXPECT_TRUE(locale->ValidateUtf8(utf8.Value()).Succeeded());

    const auto malformed = locale->DecodeUtf8("\xC0\xAF");
    EXPECT_TRUE(malformed.Failed());
    EXPECT_FALSE(malformed.Message().empty());
}

}