#pragma once

#include <future>
#include <memory>
#include <string>

#include "common.h"

namespace siesta
{
    namespace client
    {
        class Response
        {
        public:
            virtual ~Response()                          = default;
            virtual HttpStatus getStatus() const         = 0;
            virtual const std::string& getReason() const = 0;
            virtual const std::string& getBody() const   = 0;
        };

        [[nodiscard]] std::future<std::unique_ptr<Response>> getRequest(
            const std::string& address,
            const int timeout_ms);
        [[nodiscard]] std::future<std::unique_ptr<Response>> putRequest(
            const std::string& address,
            const std::string& body,
            const std::string& content_type,
            const int timeout_ms);
        [[nodiscard]] std::future<std::unique_ptr<Response>> postRequest(
            const std::string& address,
            const std::string& body,
            const std::string& content_type,
            const int timeout_ms);
        [[nodiscard]] std::future<std::unique_ptr<Response>> deleteRequest(
            const std::string& address,
            const int timeout_ms);
        [[nodiscard]] std::future<std::unique_ptr<Response>> patchRequest(
            const std::string& address,
            const std::string& body,
            const std::string& content_type,
            const int timeout_ms);

    }  // namespace client
}  // namespace siesta