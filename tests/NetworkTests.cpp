#include <Winux/Winux.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>

#include <chrono>
#include <string>

TEST(NetworkTests, FetchesExampleDomainOverHttps)
{
    auto& network = Winux::Testing::Get<Winux::Network>();
    Winux::NetworkRequest request;
    request.Url = "https://example.com/";
    request.Timeout = std::chrono::seconds(10);

    const auto result = network.Send(std::move(request));

    ASSERT_TRUE(result.Succeeded()) << result.Message();
    EXPECT_EQ(result.Value().StatusCode, 200);
    ASSERT_FALSE(result.Value().Body.empty());

    const std::string body(
        reinterpret_cast<const char*>(result.Value().Body.data()),
        result.Value().Body.size());
    EXPECT_NE(body.find("Example Domain"), std::string::npos);
}

TEST(NetworkTests, PostsRequestToHttpbin)
{
    auto& network = Winux::Testing::Get<Winux::Network>();
    Winux::NetworkRequest request;
    request.Method = "POST";
    request.Url = "https://httpbin.org/anything";
    request.Headers.emplace_back("X-Winux-Test", "request-header");
    request.Headers.emplace_back("Content-Type", "text/plain; charset=utf-8");
    constexpr std::string_view payload = "winux-network-payload";
    request.Body.reserve(payload.size());
    for (const char value : payload)
    {
        request.Body.push_back(static_cast<std::byte>(value));
    }
    request.Timeout = std::chrono::seconds(10);

    const auto result = network.Send(std::move(request));

    ASSERT_TRUE(result.Succeeded()) << result.Message();
    EXPECT_EQ(result.Value().StatusCode, 200);

    const std::string body(
        reinterpret_cast<const char*>(result.Value().Body.data()),
        result.Value().Body.size());
    EXPECT_NE(body.find("\"method\": \"POST\""), std::string::npos);
    EXPECT_NE(body.find("\"data\": \"winux-network-payload\""), std::string::npos);
    EXPECT_NE(body.find("\"X-Winux-Test\": \"request-header\""), std::string::npos);
}

TEST(NetworkTests, RejectsEmptyUrl)
{
    auto& network = Winux::Testing::Get<Winux::Network>();
    Winux::NetworkRequest request;

    const auto result = network.Send(std::move(request));

    EXPECT_TRUE(result.Failed());
    EXPECT_FALSE(result.Message().empty());
}

TEST(NetworkTests, RejectsHeaderLineBreaks)
{
    auto& network = Winux::Testing::Get<Winux::Network>();
    Winux::NetworkRequest request;
    request.Url = "http://127.0.0.1/";
    request.Headers.emplace_back("X-Test", "value\r\nInjected: true");

    const auto result = network.Send(std::move(request));

    EXPECT_TRUE(result.Failed());
    EXPECT_EQ(result.Message(), "A request header is invalid.");
}