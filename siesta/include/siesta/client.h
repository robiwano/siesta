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

        [[nodiscard]] Response getRequest(const std::string& uri,
                                          const int timeout_ms);
        [[nodiscard]] Response putRequest(const std::string& uri,
                                          const std::string& body,
                                          const std::string& content_type,
                                          const int timeout_ms);
        [[nodiscard]] Response postRequest(const std::string& uri,
                                           const std::string& body,
                                           const std::string& content_type,
                                           const int timeout_ms);
        [[nodiscard]] Response deleteRequest(const std::string& uri,
                                             const int timeout_ms);
        [[nodiscard]] Response patchRequest(const std::string& uri,
                                            const std::string& body,
                                            const std::string& content_type,
                                            const int timeout_ms);

    }  // namespace client
}  // namespace siesta