#include <Winux/Winux.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

unsigned char HexDigit(char character)
{
    if (character >= '0' && character <= '9')
    {
        return static_cast<unsigned char>(character - '0');
    }
    if (character >= 'a' && character <= 'f')
    {
        return static_cast<unsigned char>(character - 'a' + 10);
    }
    return static_cast<unsigned char>(character - 'A' + 10);
}

std::vector<std::byte> BytesFromHex(std::string_view hex)
{
    std::vector<std::byte> bytes;
    bytes.reserve(hex.size() / 2);
    for (std::size_t index = 0; index + 1 < hex.size(); index += 2)
    {
        bytes.push_back(static_cast<std::byte>((HexDigit(hex[index]) << 4) | HexDigit(hex[index + 1])));
    }
    return bytes;
}

std::string ToHex(std::span<const std::byte> bytes)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(bytes.size() * 2);
    for (const std::byte value : bytes)
    {
        const unsigned char byte = std::to_integer<unsigned char>(value);
        hex.push_back(digits[byte >> 4]);
        hex.push_back(digits[byte & 0x0f]);
    }
    return hex;
}

TEST(CryptoTests, FillsBufferWithRandomBytes)
{
    auto& crypto = Winux::Testing::Get<Winux::Crypto>();
    std::array<std::byte, 64> output{};

    const auto result = crypto.FillRandom(output);

    ASSERT_TRUE(result.Succeeded()) << result.Message();
    EXPECT_TRUE(std::any_of(output.begin(), output.end(), [](std::byte value)
    {
        return value != std::byte{};
    }));
}

TEST(CryptoTests, EmptyBufferSucceeds)
{
    auto& crypto = Winux::Testing::Get<Winux::Crypto>();
    const auto result = crypto.FillRandom({});

    EXPECT_TRUE(result.Succeeded()) << result.Message();
}

TEST(CryptoTests, SupportsBuffersLargerThanOneDrbgRequest)
{
    auto& crypto = Winux::Testing::Get<Winux::Crypto>();
    std::array<std::byte, 2048> output{};

    const auto result = crypto.FillRandom(output);

    ASSERT_TRUE(result.Succeeded()) << result.Message();
    EXPECT_TRUE(std::any_of(output.begin(), output.end(), [](std::byte value)
    {
        return value != std::byte{};
    }));
}

}

TEST(CryptoTests, Sha256MatchesKnownAnswer)
{
    auto& crypto = Winux::Testing::Get<Winux::Crypto>();
    constexpr std::string_view message = "abc";
    const auto digest = crypto.Sha256(std::as_bytes(
        std::span<const char>(message.data(), message.size())));

    ASSERT_TRUE(digest.Succeeded()) << digest.Message();
    EXPECT_EQ(ToHex(digest.Value()),
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(CryptoTests, HmacSha256MatchesKnownAnswer)
{
    auto& crypto = Winux::Testing::Get<Winux::Crypto>();
    const std::vector<std::byte> key(20, std::byte{0x0b});
    constexpr std::string_view message = "Hi There";
    const auto tag = crypto.HmacSha256(
        key,
        std::as_bytes(std::span<const char>(message.data(), message.size())));

    ASSERT_TRUE(tag.Succeeded()) << tag.Message();
    EXPECT_EQ(ToHex(tag.Value()),
        "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
}

TEST(CryptoTests, HkdfSha256MatchesRfc5869CaseOne)
{
    auto& crypto = Winux::Testing::Get<Winux::Crypto>();
    const std::vector<std::byte> input_key_material = BytesFromHex(
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
    const std::vector<std::byte> salt = BytesFromHex("000102030405060708090a0b0c");
    const std::vector<std::byte> info = BytesFromHex("f0f1f2f3f4f5f6f7f8f9");
    std::array<std::byte, 42> output{};

    const auto result = crypto.HkdfSha256(
        input_key_material,
        salt,
        info,
        output);

    ASSERT_TRUE(result.Succeeded()) << result.Message();
    EXPECT_EQ(ToHex(output),
        "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865");
}

TEST(CryptoTests, HkdfRejectsOutputLongerThanRfcLimit)
{
    auto& crypto = Winux::Testing::Get<Winux::Crypto>();
    std::vector<std::byte> output(8161);

    const auto result = crypto.HkdfSha256({}, {}, {}, output);

    EXPECT_TRUE(result.Failed());
}