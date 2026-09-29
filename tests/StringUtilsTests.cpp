#include <Winux/Winux.h>

#include <gtest/gtest.h>

TEST(StringUtilsTests, ConvertsBetweenWideAndFixedWidthUnicodeStrings)
{
    const std::u32string utf32 = U"A\U0001F642";
    const std::u16string utf16 = {u'A', static_cast<char16_t>(0xD83D), static_cast<char16_t>(0xDE42)};
    const auto utf8 = String::EncodeUtf8(utf32);
    ASSERT_TRUE(utf8.succeeded()) << utf8.message();

    const auto wide = String::FromUtf8(utf8.value());
    ASSERT_TRUE(wide.succeeded()) << wide.message();
    EXPECT_EQ(String::ToUtf8(utf16).value(), utf8.value());
    EXPECT_EQ(String::ToUtf8(utf32).value(), utf8.value());
    EXPECT_EQ(String::ToUtf8(std::wstring_view(wide.value())).value(), utf8.value());

    EXPECT_EQ(String::ToUtf16(utf8.value()).value(), utf16);
    EXPECT_EQ(String::ToUtf16(utf32).value(), utf16);
    EXPECT_EQ(String::ToUtf16(std::wstring_view(wide.value())).value(), utf16);

    EXPECT_EQ(String::ToUtf32(utf8.value()).value(), utf32);
    EXPECT_EQ(String::ToUtf32(utf16).value(), utf32);
    EXPECT_EQ(String::ToUtf32(std::wstring_view(wide.value())).value(), utf32);
    EXPECT_EQ(String::FromUtf16(utf16).value(), wide.value());
    EXPECT_EQ(String::FromUtf32(utf32).value(), wide.value());

    EXPECT_EQ(String::DecodeUtf8(utf8.value()).value(), utf32);
    EXPECT_TRUE(String::ValidateUtf8(utf8.value()).succeeded());
    EXPECT_EQ(String::EncodeUtf16(utf32).value(), utf16);
    EXPECT_EQ(String::DecodeUtf16(utf16).value(), utf32);
    EXPECT_TRUE(String::ValidateUtf16(utf16).succeeded());
    EXPECT_EQ(String::EncodeUtf32(utf32).value(), utf32);
    EXPECT_EQ(String::DecodeUtf32(utf32).value(), utf32);
    EXPECT_TRUE(String::ValidateUtf32(utf32).succeeded());
}
