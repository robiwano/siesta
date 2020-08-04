#include <nng/nng.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/tls/tls.h>
#include <siesta/client.h>

#include <string.h>

#include <vector>

using namespace siesta;
using namespace siesta::client;

namespace
{
    static void fatal(const std::string& msg, int rv)
    {
        throw std::runtime_error(msg + ": " + std::string(nng_strerror(rv)));
    }

    Response doRequest(HttpMethod method,
                       const std::string& address,
                       const std::string& body,
                       const std::string& content_type,
                       const int timeout_ms)
    {
        auto f = std::async(std::launch::async, [=]() -> std::string {
            int rv;
            static const char* method_str[] = {
                "POST",
                "PUT",
                "GET",
                "PATCH",
                "DELETE",
            };

            nng_smart_ptr<nng_url> url(nng_url_free);
            if ((rv = nng_url_parse(&url, address.c_str())) != 0) {
                fatal("Failed to parse address", rv);
            }

            nng_smart_ptr<nng_http_client> client(nng_http_client_free);
            if ((rv = nng_http_client_alloc(&client, url)) != 0) {
                fatal("Failed to alloc http client", rv);
            }

            nng_smart_ptr<nng_tls_config> tls(nng_tls_config_free);
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

            nng_smart_ptr<nng_http_req> req(nng_http_req_free);
            if ((rv = nng_http_req_alloc(&req, url)) != 0) {
                fatal("Failed to alloc http request", rv);
            }

            nng_smart_ptr<nng_http_res> res(nng_http_res_free);
            if ((rv = nng_http_res_alloc(&res)) != 0) {
                fatal("Failed to alloc http response", rv);
            }

            nng_smart_ptr<nng_aio> aio(nng_aio_free);
            if ((rv = nng_aio_alloc(&aio, NULL, NULL)) != 0) {
                fatal("Failed to alloc aio object", rv);
            }

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
            auto conn = (nng_http_conn*)nng_aio_get_output(aio, 0);

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

    struct WriterImpl : siesta::client::websocket::Writer {
        nng_smart_ptr<nng_stream_dialer> dialer{nng_stream_dialer_free};
        nng_smart_ptr<nng_aio> aio_dialer{nng_aio_free};
        nng_smart_ptr<nng_aio> aio_read{nng_aio_free};
        nng_smart_ptr<nng_aio> aio_write{nng_aio_free};
        nng_smart_ptr<nng_stream> stream{nng_stream_free};
        std::vector<uint8_t> buffer;
        std::function<void(const std::string&)> reader;
        WriterImpl(const std::string& address,
                   std::function<void(const std::string&)> r)
            : reader(r), buffer(32768)
        {
            int rv;
            if ((rv = nng_stream_dialer_alloc(&dialer, address.c_str())) != 0) {
                fatal("nng_stream_dialer_alloc", rv);
            }
            if ((rv = nng_aio_alloc(&aio_dialer, nullptr, nullptr)) != 0) {
                fatal("nng_aio_alloc", rv);
            }
            if ((rv = nng_aio_alloc(
                     &aio_read,
                     [](void* arg) { ((WriterImpl*)arg)->read_cb(); },
                     this)) != 0) {
                fatal("nng_aio_alloc", rv);
            }
            if ((rv = nng_aio_alloc(&aio_write, nullptr, nullptr)) != 0) {
                fatal("nng_aio_alloc", rv);
            }
            nng_stream_dialer_dial(dialer, aio_dialer);
            nng_aio_wait(aio_dialer);
            rv = nng_aio_result(aio_dialer);
            if (rv != 0) {
                fatal("dial", rv);
            }
            stream = (nng_stream*)nng_aio_get_output(aio_dialer, 0);
            startRead();
        }
        ~WriterImpl()
        {
            nng_aio_cancel(aio_dialer);
            nng_aio_cancel(aio_read);
            nng_aio_cancel(aio_write);
            nng_stream_dialer_close(dialer);
        }

        void startRead()
        {
            nng_iov iov{buffer.data(), buffer.size()};
            nng_aio_set_iov(aio_read, 1, &iov);
            nng_stream_recv(stream, aio_read);
        }

        void read_cb()
        {
            int rv = nng_aio_result(aio_read);
            if (rv != 0) {
                return;
            }
            auto len = nng_aio_count(aio_read);
            std::string data((const char*)buffer.data(), len);
            reader(data);
            startRead();
        }

        void writeData(const std::string& data) override
        {
            nng_iov iov{(void*)data.data(), data.size()};
            nng_aio_set_iov(aio_write, 1, &iov);
            nng_stream_send(stream, aio_write);
            nng_aio_wait(aio_write);
            int rv = nng_aio_result(aio_write);
            if (rv != 0) {
                fatal("nng_aio_result", rv);
            }
        }
    };
}  // namespace

siesta::client::Response siesta::client::getRequest(const std::string& address,
                                                    const int timeout_ms)
{
    return doRequest(HttpMethod::GET, address, "", "", timeout_ms);
}

Response siesta::client::putRequest(const std::string& address,
                                    const std::string& body,
                                    const std::string& content_type,
                                    const int timeout_ms)
{
    return doRequest(HttpMethod::PUT, address, body, content_type, timeout_ms);
}

Response siesta::client::postRequest(const std::string& address,
                                     const std::string& body,
                                     const std::string& content_type,
                                     const int timeout_ms)
{
    return doRequest(HttpMethod::POST, address, body, content_type, timeout_ms);
}

Response siesta::client::deleteRequest(const std::string& address,
                                       const int timeout_ms)
{
    return doRequest(HttpMethod::DEL, address, "", "", timeout_ms);
}

Response siesta::client::patchRequest(const std::string& address,
                                      const std::string& body,
                                      const std::string& content_type,
                                      const int timeout_ms)
{
    return doRequest(
        HttpMethod::PATCH, address, body, content_type, timeout_ms);
}

std::unique_ptr<siesta::client::websocket::Writer>
siesta::client::websocket::connect(
    const std::string& uri,
    std::function<void(const std::string&)> reader)
{
    return std::unique_ptr<siesta::client::websocket::Writer>(
        new WriterImpl(uri, reader));
}
