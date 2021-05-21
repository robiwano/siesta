#pragma once

#include <future>
#include <memory>
#include <string>

#include "common.h"

namespace siesta
{
    namespace client
    {
        using Response = std::future<std::string>;

        NO_DISCARD Response getRequest(
            const std::string& address,
            const std::vector<std::pair<std::string, std::string>> headers =
                std::vector<std::pair<std::string, std::string>>(),
            const int timeout_ms = 1000);
        NO_DISCARD Response putRequest(const std::string& uri,
                                       const std::string& body,
                                       const std::string& content_type,
            const std::vector<std::pair<std::string, std::string>> headers =
                std::vector<std::pair<std::string, std::string>>(),
                                       const int timeout_ms = 1000);
        NO_DISCARD Response postRequest(const std::string& uri,
                                        const std::string& body,
                                        const std::string& content_type,
            const std::vector<std::pair<std::string, std::string>> headers =
                std::vector<std::pair<std::string, std::string>>(),
                                        const int timeout_ms = 1000);
        NO_DISCARD Response deleteRequest(const std::string& uri,
            const std::vector<std::pair<std::string, std::string>> headers =
                std::vector<std::pair<std::string, std::string>>(),
                                          const int timeout_ms = 1000);
        NO_DISCARD Response patchRequest(const std::string& uri,
                                         const std::string& body,
                                         const std::string& content_type,
            const std::vector<std::pair<std::string, std::string>> headers =
                std::vector<std::pair<std::string, std::string>>(),
                                         const int timeout_ms = 1000);

        namespace websocket
        {
            class Writer
            {
            public:
                virtual ~Writer()                               = default;
                virtual void writeData(const std::string& data) = 0;
            };

            std::unique_ptr<Writer> connect(
                const std::string& uri,
                std::function<void(const std::string&)> reader);
        }  // namespace websocket
    }      // namespace client
}  // namespace siesta
