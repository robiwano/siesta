#pragma once

#if __cplusplus >= 201703L
#define NO_DISCARD [[nodiscard]]
#else
#define NO_DISCARD
#endif

namespace siesta
{
    enum class HttpMethod {
        POST,
        PUT,
        GET,
        PATCH,
        DEL,
        Method_COUNT_DO_NOT_USE,
    };

    enum class HttpStatus {
        CONTINUE                 = 100,
        SWITCHING                = 101,
        PROCESSING               = 102,
        OK                       = 200,
        CREATED                  = 201,
        ACCEPTED                 = 202,
        NOT_AUTHORITATIVE        = 203,
        NO_CONTENT               = 204,
        RESET_CONTENT            = 205,
        PARTIAL_CONTENT          = 206,
        MULTI_STATUS             = 207,
        ALREADY_REPORTED         = 208,
        IM_USED                  = 226,
        MULTIPLE_CHOICES         = 300,
        STATUS_MOVED_PERMANENTLY = 301,
        FOUND                    = 302,
        SEE_OTHER                = 303,
        NOT_MODIFIED             = 304,
        USE_PROXY                = 305,
        TEMPORARY_REDIRECT       = 307,
        PERMANENT_REDIRECT       = 308,
        BAD_REQUEST              = 400,
        UNAUTHORIZED             = 401,
        PAYMENT_REQUIRED         = 402,
        FORBIDDEN                = 403,
        NOT_FOUND                = 404,
        METHOD_NOT_ALLOWED       = 405,
        NOT_ACCEPTABLE           = 406,
        PROXY_AUTH_REQUIRED      = 407,
        REQUEST_TIMEOUT          = 408,
        CONFLICT                 = 409,
        GONE                     = 410,
        LENGTH_REQUIRED          = 411,
        PRECONDITION_FAILED      = 412,
        PAYLOAD_TOO_LARGE        = 413,
        ENTITY_TOO_LONG          = 414,
        UNSUPPORTED_MEDIA_TYPE   = 415,
        RANGE_NOT_SATISFIABLE    = 416,
        EXPECTATION_FAILED       = 417,
        TEAPOT                   = 418,
        UNPROCESSABLE_ENTITY     = 422,
        LOCKED                   = 423,
        FAILED_DEPENDENCY        = 424,
        UPGRADE_REQUIRED         = 426,
        PRECONDITION_REQUIRED    = 428,
        TOO_MANY_REQUESTS        = 429,
        HEADERS_TOO_LARGE        = 431,
        UNAVAIL_LEGAL_REASONS    = 451,
        INTERNAL_SERVER_ERROR    = 500,
        NOT_IMPLEMENTED          = 501,
        BAD_GATEWAY              = 502,
        SERVICE_UNAVAILABLE      = 503,
        GATEWAY_TIMEOUT          = 504,
        HTTP_VERSION_NOT_SUPP    = 505,
        VARIANT_ALSO_NEGOTIATES  = 506,
        INSUFFICIENT_STORAGE     = 507,
        LOOP_DETECTED            = 508,
        NOT_EXTENDED             = 510,
        NETWORK_AUTH_REQUIRED    = 511,
    };

    class Exception : public std::exception
    {
        HttpStatus status_;
        std::string reason_;

    public:
        explicit Exception(HttpStatus status, const std::string& reason = "")
            : status_(status), reason_(reason)
        {
        }

        ~Exception() _NOEXCEPT = default;

        HttpStatus status() const { return status_; }
        bool has_reason() const { return !reason_.empty(); }
        const char* what() const _NOEXCEPT override { return reason_.c_str(); }
    };

    template <class nng_type,
              class nng_free_function = std::function<void(nng_type*)>>
    class nng_smart_ptr
    {
        nng_type* obj{nullptr};
        nng_free_function fn_free;
        void release()
        {
            if (obj != nullptr) {
                fn_free(obj);
                obj = nullptr;
            }
        }

    public:
        nng_smart_ptr(nng_free_function fn) : fn_free(fn) {}
        ~nng_smart_ptr() { release(); }
        nng_smart_ptr& operator=(nng_type* new_obj)
        {
            release();
            obj = new_obj;
            return *this;
        }
        nng_type** operator&() { return &obj; }
        operator nng_type*() const { return obj; }
        nng_type* operator->() { return obj; }
    };

}  // namespace siesta