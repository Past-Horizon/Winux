#pragma once

#include <Winux/Core/Results.h>

#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Winux::Contracts {

/*
    @summary
    Sends HTTP requests and returns their responses.
*/
class INetwork {
public:
    using Header = std::pair<std::string, std::string>;
    using Headers = std::vector<Header>;

    /*
        @summary
        Describes one HTTP request.
    */
    struct Request {
        std::string Method = "GET";
        std::string Url;
        Headers Headers;
        std::vector<std::byte> Body;
        std::chrono::milliseconds Timeout{30000};
        bool FollowRedirects = false;
    };

    /*
        @summary
        Contains an HTTP response.
    */
    struct Response {
        long StatusCode = 0;
        Headers Headers;
        std::vector<std::byte> Body;
    };

    virtual ~INetwork() = default;

    /*
        @summary
        Sends one request using the current synchronous implementation.

        @param request
        Owned request data, including its method, URL, headers, body, and timeout.

        @returns
        The HTTP response for any received status code, or a failure for invalid input or a transport error.

        @warning
        Is synchronous, later revision will change it to async
    */
    virtual Core::Result<Response> Send(Request request);
};

}