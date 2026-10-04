#include <Winux/Contracts/INetwork.h>

#include <curl/curl.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <mutex>
#include <string_view>

namespace {

class CurlGlobalState
{
public:
    CurlGlobalState()
        : result_(curl_global_init(CURL_GLOBAL_DEFAULT))
    {
    }

    ~CurlGlobalState()
    {
        if (result_ == CURLE_OK)
        {
            curl_global_cleanup();
        }
    }

    CURLcode Result() const noexcept
    {
        return result_;
    }

private:
    CURLcode result_;
};

struct CurlResponseBuffer
{
    Winux::Contracts::INetwork::Response& response;
};

std::size_t AppendBody(char* data, std::size_t size, std::size_t count, void* context) noexcept
{
    if (size != 0 && count > (std::numeric_limits<std::size_t>::max)() / size)
    {
        return 0;
    }

    const std::size_t length = size * count;
    auto& response = static_cast<CurlResponseBuffer*>(context)->response;
    try
    {
        const auto* begin = reinterpret_cast<const std::byte*>(data);
        response.Body.insert(response.Body.end(), begin, begin + length);
        return length;
    }
    catch (...)
    {
        return 0;
    }
}

std::size_t AppendHeader(char* data, std::size_t size, std::size_t count, void* context) noexcept
{
    if (size != 0 && count > (std::numeric_limits<std::size_t>::max)() / size)
    {
        return 0;
    }

    const std::size_t length = size * count;
    auto& response = static_cast<CurlResponseBuffer*>(context)->response;
    try
    {
        std::string_view line(data, length);
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        {
            line.remove_suffix(1);
        }

        if (line.starts_with("HTTP/"))
        {
            response.Headers.clear();
            return length;
        }

        const std::size_t separator = line.find(':');
        if (separator == std::string_view::npos)
        {
            return length;
        }

        std::string_view value = line.substr(separator + 1);
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
        {
            value.remove_prefix(1);
        }
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
        {
            value.remove_suffix(1);
        }

        response.Headers.emplace_back(
            std::string(line.substr(0, separator)),
            std::string(value));
        return length;
    }
    catch (...)
    {
        return 0;
    }
}

bool ContainsUnsafeHeaderCharacter(std::string_view value)
{
    return value.find_first_of("\r\n\0", 0, 3) != std::string_view::npos;
}

Winux::Core::Result<Winux::Contracts::INetwork::Response> SendWithCurl(
    Winux::Contracts::INetwork::Request request)
{
    using Response = Winux::Contracts::INetwork::Response;

    if (request.Url.empty() || request.Url.find('\0') != std::string::npos)
    {
        return Winux::Core::Result<Response>::Failure("The request URL must not be empty or contain null characters.");
    }
    if (request.Method.empty() || ContainsUnsafeHeaderCharacter(request.Method)
        || request.Method.find_first_of(" \t") != std::string::npos)
    {
        return Winux::Core::Result<Response>::Failure("The request method is invalid.");
    }
    if (request.Timeout.count() <= 0
        || static_cast<std::uint64_t>(request.Timeout.count())
            > static_cast<std::uint64_t>((std::numeric_limits<long>::max)()))
    {
        return Winux::Core::Result<Response>::Failure("The request timeout must be positive and fit in a long.");
    }

    for (const auto& [name, value] : request.Headers)
    {
        if (name.empty() || name.find(':') != std::string::npos
            || ContainsUnsafeHeaderCharacter(name) || ContainsUnsafeHeaderCharacter(value))
        {
            return Winux::Core::Result<Response>::Failure("A request header is invalid.");
        }
    }

    static CurlGlobalState global_state;
    if (global_state.Result() != CURLE_OK)
    {
        return Winux::Core::Result<Response>::Failure(
            std::string("cURL initialization failed: ") + curl_easy_strerror(global_state.Result()));
    }

    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(curl_easy_init(), curl_easy_cleanup);
    if (!handle)
    {
        return Winux::Core::Result<Response>::Failure("cURL could not create an HTTP request handle.");
    }

    Response response;
    CurlResponseBuffer response_buffer{response};
    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(nullptr, curl_slist_free_all);
    for (const auto& [name, value] : request.Headers)
    {
        const std::string header = name + ": " + value;
        curl_slist* appended = curl_slist_append(headers.get(), header.c_str());
        if (appended == nullptr)
        {
            return Winux::Core::Result<Response>::Failure("cURL could not allocate the request headers.");
        }
        headers.release();
        headers.reset(appended);
    }

    char error_buffer[CURL_ERROR_SIZE]{};
    const auto set_option = [&handle](CURLoption option, auto value)
    {
        return curl_easy_setopt(handle.get(), option, value);
    };
    const auto option_error = [](CURLcode result, const char* operation)
    {
        if (result == CURLE_OK)
        {
            return std::string{};
        }
        return std::string("cURL could not configure ") + operation + ": " + curl_easy_strerror(result);
    };

    CURLcode result = set_option(CURLOPT_ERRORBUFFER, error_buffer);
    if (result == CURLE_OK) result = set_option(CURLOPT_URL, request.Url.c_str());
    if (result == CURLE_OK) result = set_option(CURLOPT_CUSTOMREQUEST, request.Method.c_str());
    if (result == CURLE_OK) result = set_option(CURLOPT_NOSIGNAL, 1L);
    if (result == CURLE_OK) result = set_option(CURLOPT_TIMEOUT_MS, static_cast<long>(request.Timeout.count()));
    if (result == CURLE_OK) result = set_option(CURLOPT_FOLLOWLOCATION, request.FollowRedirects ? 1L : 0L);
    if (result == CURLE_OK) result = set_option(CURLOPT_MAXREDIRS, 10L);
    if (result == CURLE_OK) result = set_option(CURLOPT_SSL_VERIFYPEER, 1L);
    if (result == CURLE_OK) result = set_option(CURLOPT_SSL_VERIFYHOST, 2L);
    if (result == CURLE_OK) result = set_option(CURLOPT_WRITEFUNCTION, &AppendBody);
    if (result == CURLE_OK) result = set_option(CURLOPT_WRITEDATA, &response_buffer);
    if (result == CURLE_OK) result = set_option(CURLOPT_HEADERFUNCTION, &AppendHeader);
    if (result == CURLE_OK) result = set_option(CURLOPT_HEADERDATA, &response_buffer);
    if (result == CURLE_OK && headers)
    {
        result = set_option(CURLOPT_HTTPHEADER, headers.get());
    }
    if (result == CURLE_OK && !request.Body.empty())
    {
        result = set_option(CURLOPT_POSTFIELDS, request.Body.data());
        if (result == CURLE_OK)
        {
            result = set_option(
                CURLOPT_POSTFIELDSIZE_LARGE,
                static_cast<curl_off_t>(request.Body.size()));
        }
    }
    if (result != CURLE_OK)
    {
        return Winux::Core::Result<Response>::Failure(option_error(result, "the request"));
    }

    result = curl_easy_perform(handle.get());
    if (result != CURLE_OK)
    {
        const std::string message = error_buffer[0] != '\0'
            ? error_buffer
            : curl_easy_strerror(result);
        return Winux::Core::Result<Response>::Failure(std::string("cURL request failed: ") + message);
    }

    result = curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &response.StatusCode);
    if (result != CURLE_OK)
    {
        return Winux::Core::Result<Response>::Failure(
            std::string("cURL could not read the HTTP response status: ") + curl_easy_strerror(result));
    }

    return Winux::Core::Result<Response>::Success(std::move(response));
}

}

namespace Winux::Contracts {

Core::Result<INetwork::Response> INetwork::Send(Request request)
{
    return SendWithCurl(std::move(request));
}

}