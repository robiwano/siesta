#pragma once

#include <future>
#include <memory>
#include <string>

#include "common.h"

namespace siesta
{
    namespace client
    {
        class ClientException : public std::exception
        {
            HttpStatus status_;
            std::string what_;

        public:
            ClientException(HttpStatus status, const std::string& reason);
            HttpStatus status() const;
            const char* what() const noexcept override;
        };

        using Response = std::future<std::string>;

        [[nodiscard]] Response getRequest(const std::string& address,
                                          const int timeout_ms);
        [[nodiscard]] Response putRequest(const std::string& address,
                                          const std::string& body,
                                          const std::string& content_type,
                                          const int timeout_ms);
        [[nodiscard]] Response postRequest(const std::string& address,
                                           const std::string& body,
                                           const std::string& content_type,
                                           const int timeout_ms);
        [[nodiscard]] Response deleteRequest(const std::string& address,
                                             const int timeout_ms);
        [[nodiscard]] Response patchRequest(const std::string& address,
                                            const std::string& body,
                                            const std::string& content_type,
                                            const int timeout_ms);

    }  // namespace client
}  // namespace siesta