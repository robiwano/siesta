#pragma once

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "common.h"

namespace siesta
{
    namespace client
    {
        using Response = std::future<std::string>;
        using Headers  = std::vector<std::pair<std::string, std::string>>;

        NO_DISCARD Response getRequest(const std::string& address,
                                       const Headers& headers = Headers(),
                                       const int timeout_ms   = 1000);
        NO_DISCARD Response putRequest(const std::string& uri,
                                       const std::string& body,
                                       const std::string& content_type,
                                       const Headers& headers = Headers(),
                                       const int timeout_ms   = 1000);
        NO_DISCARD Response postRequest(const std::string& uri,
                                        const std::string& body,
                                        const std::string& content_type,
                                        const Headers& headers = Headers(),
                                        const int timeout_ms   = 1000);
        NO_DISCARD Response deleteRequest(const std::string& uri,
                                          const Headers& headers = Headers(),
                                          const int timeout_ms   = 1000);
        NO_DISCARD Response patchRequest(const std::string& uri,
                                         const std::string& body,
                                         const std::string& content_type,
                                         const Headers& headers = Headers(),
                                         const int timeout_ms   = 1000);

        namespace websocket
        {
            class Writer
            {
            public:
                virtual ~Writer()                          = default;
                virtual void send(const std::string& data) = 0;
            };

            std::unique_ptr<Writer> connect(
                const std::string& uri,
                std::function<void(Writer&, const std::string&)> on_message,
                std::function<void(Writer&)> on_open = nullptr,
                std::function<void(Writer&, const std::string&)> on_error =
                    nullptr,
                std::function<void(Writer&)> on_close = nullptr,
                const bool text_mode                  = true);
        }  // namespace websocket
    }      // namespace client
}  // namespace siesta
