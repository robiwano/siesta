#include <nng/nng.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/tls/tls.h>
#include <siesta/client.h>

#include <string.h>

using namespace siesta;
using namespace siesta::client;

namespace
{
    template <typename Type>
    using Deleter = std::unique_ptr<Type, void (*)(Type*)>;

    static void fatal(const std::string& msg, int rv)
    {
        throw std::runtime_error(msg + ": " + std::string(nng_strerror(rv)));
    }

    Response doRequest(Method method,
                       const std::string& address,
                       const std::string& body,
                       const std::string& content_type,
                       const int timeout_ms)
    {
        auto f = std::async(std::launch::async, [=]() -> std::string {
            nng_http_client* client;
            nng_http_conn* conn;
            nng_url* url;
            nng_aio* aio;
            nng_http_req* req;
            nng_http_res* res;
            nng_tls_config* tls = NULL;
            int rv;
            static const char* method_str[] = {
                "POST",
                "PUT",
                "GET",
                "PATCH",
                "DELETE",
            };

            if ((rv = nng_url_parse(&url, address.c_str())) != 0) {
                fatal("Failed to parse address", rv);
            }
            Deleter<nng_url> free_url(url, nng_url_free);

            if ((rv = nng_http_client_alloc(&client, url)) != 0) {
                fatal("Failed to alloc http client", rv);
            }
            Deleter<nng_http_client> free_client(client, nng_http_client_free);

            if (strcmp(url->u_scheme, "https") == 0) {
                if ((rv = nng_tls_config_alloc(&tls, NNG_TLS_MODE_CLIENT)) !=
                    0) {
                    fatal("nng_tls_config_alloc", rv);
                }

                if ((rv = nng_tls_config_server_name(tls, url->u_hostname)) !=
                    0) {
                    fatal("nng_tls_config_server_name", rv);
                }

                if ((rv = nng_tls_config_auth_mode(
                         tls, NNG_TLS_AUTH_MODE_NONE)) != 0) {
                    fatal("nng_tls_config_alloc", rv);
                }

                if ((rv = nng_http_client_set_tls(client, tls)) != 0) {
                    fatal("nng_http_client_set_tls", rv);
                }
            }
            Deleter<nng_tls_config> free_tls(tls, nng_tls_config_free);

            if ((rv = nng_http_req_alloc(&req, url)) != 0) {
                fatal("Failed to alloc http request", rv);
            }
            Deleter<nng_http_req> free_req(req, nng_http_req_free);

            if ((rv = nng_http_res_alloc(&res)) != 0) {
                fatal("Failed to alloc http response", rv);
            }
            Deleter<nng_http_res> free_resp(res, nng_http_res_free);

            if ((rv = nng_aio_alloc(&aio, NULL, NULL)) != 0) {
                fatal("Failed to alloc aio object", rv);
            }
            Deleter<nng_aio> free_aio(aio, nng_aio_free);

            nng_aio_set_timeout(
                aio, timeout_ms >= 0 ? timeout_ms : NNG_DURATION_DEFAULT);

            // Start connection process...
            nng_http_client_connect(client, aio);

            // Wait for it to finish.
            nng_aio_wait(aio);
            if ((rv = nng_aio_result(aio)) != 0) {
                fatal("Failed getting aio object result", rv);
            }

            nng_aio_set_timeout(aio, NNG_DURATION_DEFAULT);
            // Get the connection, at the 0th output.
            conn = (nng_http_conn*)nng_aio_get_output(aio, 0);

            // Request is already set up with URL, and for GET via HTTP/1.1.
            // The Host: header is already set up too.
            if ((rv = nng_http_req_set_method(req, method_str[int(method)])) !=
                0) {
                fatal("Failed setting method", rv);
            }

            if (!body.empty()) {
                if ((rv = nng_http_req_set_data(
                         req, body.data(), body.length())) != 0) {
                    fatal("Failed setting request data", rv);
                }
                if (!content_type.empty()) {
                    if ((rv = nng_http_req_add_header(
                             req, "content-type", content_type.c_str())) != 0) {
                        fatal("Failed setting content type header", rv);
                    }
                }
            }

            nng_aio_set_timeout(
                aio, timeout_ms >= 0 ? timeout_ms : NNG_DURATION_DEFAULT);
            // Send the request, and wait for that to finish.
            nng_http_conn_write_req(conn, req, aio);
            nng_aio_wait(aio);

            if ((rv = nng_aio_result(aio)) != 0) {
                fatal("Failed getting aio object result", rv);
            }

            // Read a response.
            nng_http_conn_read_res(conn, res, aio);
            nng_aio_wait(aio);

            if ((rv = nng_aio_result(aio)) != 0) {
                fatal("Failed getting aio object result", rv);
            }

            std::string r;
            if (nng_http_res_get_status(res) != NNG_HTTP_STATUS_OK) {
                throw siesta::Exception(
                    static_cast<HttpStatus>(nng_http_res_get_status(res)),
                    nng_http_res_get_reason(res));
            } else {
                const char* hdr;

                // This only supports regular transfer encoding (no
                // Chunked-Encoding, and a Content-Length header is
                // required.)
                if ((hdr = nng_http_res_get_header(res, "Content-Length")) ==
                    NULL) {
                    throw std::runtime_error("Missing Content-Length header");
                }

                int len = atoi(hdr);
                if (len > 0) {
                    r.resize(len);
                    nng_iov iov;

                    // Set up a single iov to point to the buffer.
                    iov.iov_len = len;
                    iov.iov_buf = (void*)r.data();

                    // Following never fails with fewer than 5 elements.
                    nng_aio_set_iov(aio, 1, &iov);

                    // Now attempt to receive the data.
                    nng_http_conn_read_all(conn, aio);

                    // Wait for it to complete.
                    nng_aio_wait(aio);

                    if ((rv = nng_aio_result(aio)) != 0) {
                        fatal("Failed getting aio object result", rv);
                    }
                }
            }
            return r;
        });
        return f;
    }
}  // namespace

siesta::client::Response siesta::client::getRequest(const std::string& address,
                                                    const int timeout_ms)
{
    return doRequest(Method::GET, address, "", "", timeout_ms);
}

Response siesta::client::putRequest(const std::string& address,
                                    const std::string& body,
                                    const std::string& content_type,
                                    const int timeout_ms)
{
    return doRequest(Method::PUT, address, body, content_type, timeout_ms);
}

Response siesta::client::postRequest(const std::string& address,
                                     const std::string& body,
                                     const std::string& content_type,
                                     const int timeout_ms)
{
    return doRequest(Method::POST, address, body, content_type, timeout_ms);
}

Response siesta::client::deleteRequest(const std::string& address,
                                       const int timeout_ms)
{
    return doRequest(Method::DELETE, address, "", "", timeout_ms);
}

Response siesta::client::patchRequest(const std::string& address,
                                      const std::string& body,
                                      const std::string& content_type,
                                      const int timeout_ms)
{
    return doRequest(Method::PATCH, address, body, content_type, timeout_ms);
}
