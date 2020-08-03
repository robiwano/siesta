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

        NO_DISCARD Response getRequest(const std::string& uri,
                                       const int timeout_ms);
        NO_DISCARD Response putRequest(const std::string& uri,
                                       const std::string& body,
                                       const std::string& content_type,
                                       const int timeout_ms);
        NO_DISCARD Response postRequest(const std::string& uri,
                                        const std::string& body,
                                        const std::string& content_type,
                                        const int timeout_ms);
        NO_DISCARD Response deleteRequest(const std::string& uri,
                                          const int timeout_ms);
        NO_DISCARD Response patchRequest(const std::string& uri,
                                         const std::string& body,
                                         const std::string& content_type,
                                         const int timeout_ms);

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
