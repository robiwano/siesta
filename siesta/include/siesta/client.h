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

    }  // namespace client
}  // namespace siesta