#include <Winux/Winux.h>

#include <gtest/gtest.h>

TEST(StringUtilsTests, ConvertsBetweenWideAndFixedWidthUnicodeStrings)
{
    const std::u32string utf32 = U"A\U0001F642";
    const std::u16string utf16 = {u'A', static_cast<char16_t>(0xD83D), static_cast<char16_t>(0xDE42)};
    const auto utf8 = String::EncodeUtf8(utf32);
    ASSERT_TRUE(utf8.Succeeded()) << utf8.Message();

    const auto wide = String::FromUtf8(utf8.Value());
    ASSERT_TRUE(wide.Succeeded()) << wide.Message();
    EXPECT_EQ(String::ToUtf8(utf16).Value(), utf8.Value());
    EXPECT_EQ(String::ToUtf8(utf32).Value(), utf8.Value());
    EXPECT_EQ(String::ToUtf8(std::wstring_view(wide.Value())).Value(), utf8.Value());

    EXPECT_EQ(String::ToUtf16(utf8.Value()).Value(), utf16);
    EXPECT_EQ(String::ToUtf16(utf32).Value(), utf16);
    EXPECT_EQ(String::ToUtf16(std::wstring_view(wide.Value())).Value(), utf16);

    EXPECT_EQ(String::ToUtf32(utf8.Value()).Value(), utf32);
    EXPECT_EQ(String::ToUtf32(utf16).Value(), utf32);
    EXPECT_EQ(String::ToUtf32(std::wstring_view(wide.Value())).Value(), utf32);
    EXPECT_EQ(String::FromUtf16(utf16).Value(), wide.Value());
    EXPECT_EQ(String::FromUtf32(utf32).Value(), wide.Value());

    EXPECT_EQ(String::DecodeUtf8(utf8.Value()).Value(), utf32);
    EXPECT_TRUE(String::ValidateUtf8(utf8.Value()).Succeeded());
    EXPECT_EQ(String::EncodeUtf16(utf32).Value(), utf16);
    EXPECT_EQ(String::DecodeUtf16(utf16).Value(), utf32);
    EXPECT_TRUE(String::ValidateUtf16(utf16).Succeeded());
    EXPECT_EQ(String::EncodeUtf32(utf32).Value(), utf32);
    EXPECT_EQ(String::DecodeUtf32(utf32).Value(), utf32);
    EXPECT_TRUE(String::ValidateUtf32(utf32).Succeeded());
}
