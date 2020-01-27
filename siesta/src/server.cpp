#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/tls/tls.h>
#include <nng/supplemental/util/platform.h>
#include <nng/transport/tls/tls.h>
#include <siesta/server.h>

#include <mutex>
#include <regex>
#include <stdexcept>

#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

using namespace siesta;
using namespace siesta::server;

namespace
{
    // This self-signed certificate is valid for 100 years. The CN is 127.0.0.1.
    //
    // Generated using openssl:
    //
    // % openssl req -newkey rsa:2048 -nodes -keyout key.pem
    //           -x509 -days 36500 -out certificate.pem
    //
    // Relevant metadata:
    //
    // Certificate:
    //     Data:
    //         Version: 3 (0x2)
    //         Serial Number:
    //             e3:38:92:c1:35:4b:bd:ee
    //     Signature Algorithm: sha256WithRSAEncryption
    //         Issuer: CN = 127.0.0.1
    //         Validity
    //             Not Before: Dec 26 07:26:58 2019 GMT
    //             Not After : Dec  2 07:26:58 2119 GMT
    //         Subject: CN = 127.0.0.1
    //         Subject Public Key Info:
    //             Public Key Algorithm: rsaEncryption
    //                 Public-Key: (2048 bit)
    //
    static const char tls_cert[] = R"(
-----BEGIN CERTIFICATE-----
MIIDADCCAeigAwIBAgIJAOM4ksE1S73uMA0GCSqGSIb3DQEBCwUAMBQxEjAQBgNV
BAMMCTEyNy4wLjAuMTAgFw0xOTEyMjYwNzI2NThaGA8yMTE5MTIwMjA3MjY1OFow
FDESMBAGA1UEAwwJMTI3LjAuMC4xMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEArComOWRVREMFgzSHvQvKnSBtDsxwkCV0toYEc0Fe0LwDTXZlXatL5hx2
cVerTVI++ljtlnoEomFGnAtkSbaZR8ofxEqYC53ePaa2FIdt/ymJkV0Nc752PLVO
6i9UnQ6oTmYTkmVilfuIUJMxE/om+kVFBctVgxq/7zrXBgyC1LhV5aU6YNRjBGHz
N8W49nZ/bjCu27o5qkYgUG5/LLcswh7yDKzmxM87rY2snes7qsNb6MdkmWfVjvrK
ln+4MVCKI3XknVUHASMqt0XItBiq6j4KWRls0oE5I5rNA5fg5jvLYUOz2CgvdhDO
jlX3wGJRi/EWd5TDv8j+8X0cx46MBQIDAQABo1MwUTAdBgNVHQ4EFgQUBHu6g2tY
3kCSppkONcPN5YVYnwMwHwYDVR0jBBgwFoAUBHu6g2tY3kCSppkONcPN5YVYnwMw
DwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAU9fbCV2pM4cDhmPR
9o37Qfhv9hen9dU+cytqomaNelytUBWciaKmPqsj5Zce3+Ole9gsCht6HGRmXf4C
zKHUm0G6N1ijY2B9uwQK6ux/v4aAAOimB9ZyOCJp4t20RI6hsH/FNIy9fdvUBFPy
2CYgTuPsp/P4duaAGDMgxSugNC90C86jV+Gqs7r++5NB7d7U3OsGbOrqZXgha/hf
sewq2VW57eeZiAAyHEsrR0u0tHbEBg635PbNz9scqpdJUBpicxJDV0S0q8hZYNAE
FVzPKT4CKKluYGCEz7rtYqL5NP0x3BHMmiLqjN+ApLvVJJBW+TijIMki+jxW0Lry
pbwnpQ==
-----END CERTIFICATE-----)";

    static const char tls_key[] = R"(
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCsKiY5ZFVEQwWD
NIe9C8qdIG0OzHCQJXS2hgRzQV7QvANNdmVdq0vmHHZxV6tNUj76WO2WegSiYUac
C2RJtplHyh/ESpgLnd49prYUh23/KYmRXQ1zvnY8tU7qL1SdDqhOZhOSZWKV+4hQ
kzET+ib6RUUFy1WDGr/vOtcGDILUuFXlpTpg1GMEYfM3xbj2dn9uMK7bujmqRiBQ
bn8styzCHvIMrObEzzutjayd6zuqw1vox2SZZ9WO+sqWf7gxUIojdeSdVQcBIyq3
Rci0GKrqPgpZGWzSgTkjms0Dl+DmO8thQ7PYKC92EM6OVffAYlGL8RZ3lMO/yP7x
fRzHjowFAgMBAAECggEAGPV/VyCpj9zbhrrt1sVH2WGjDdsrkmorsm5ZZNAcS8yF
+gvpBBxaQ4Dq1uGrzujWgnqz7vW/iD7r+qFYJ6uWKyctVcquojh/yJZLnUxI8Q33
iKBh297Hy2NJjn/QF3jRg5Qe0EFsemvdxjigi9HfJrc2G3Hv8rLFEoyIMbNMoEPg
DkV5UW9FMe9RJ3NyLdfj77uYZxwoBw4+mZNsIcxNcONYizMBwLXvGTmnchUG88xa
oSXsmqP9sds1KOuGf1abZ2jVqfa/zgTzGnFBDprvHWgibMDxEWv5cL7ob8SvcyKt
RpbIQd03rIa04zT7FJ2K0zQb84Og28eC9Nwb1kS4vQKBgQDem+yrDYV2kZVdtsQR
FdcMVv4LIf5AKxC/OkiAdIDdoJ837/hvrKNsKkx11YKcRhfrMhs505S4Zw/8NYcg
79HX++r8dYfn4+rA8lTZ122HJRss6mVN5ISrBQuybZcYLPwcmyTZTl2Kx05ed404
FjK0IxFf5pGeMksrzEHMN/sDZwKBgQDF/S8nYDnPIJPdBV5oqGNC7IfA8PuBj6Mp
GcIlfKBGh8k/OvnLdyHRvqyR47DVQpOcG5CcCEfh6LU9cNKrK+jWXhJor7LGqVar
JzK+virkK/oMbT0ck1711h9p8kICyz6KOt6fPiwkHDmTTs4vCZRt8LP6XnnF/xRl
4BccAkOdswKBgQCU8snRzlNN+a1yrhbUw8NHe3Gya0VfFDG5cjsO0GVlZdMDL6sQ
tfgHKOpOMdWZ0QCyG63B7INnO3ajsAFBlZXYKbSaxd1w2Ly766nAtPeRZM+hJxkv
nEb004R3GALwZzEtxtVKHbhTYnZamS3BqIC2rXwzqegnbMmFfb9M8Owg4wKBgDqD
pnEDvnIZ1bmHwaw6wANidoiucBaNhhI6m6eKmq/dp7u5SWQ51FPx/3yqh3Ov1oJX
nziONfhtV0tOUeTm+EyKxvQLoVGXcJbq4dN/zpta5+7ORjZw06riWqxsPdgni1c9
KNh1foQ5l0aTDtrWAPkxH3AKhgDfb37gaNQNU0CDAoGAcx+KByf9XVIYa5pB3uIK
z0lEkiWQ1XKmlpLODNdLZrNhbzmxfV8JwpzstpsIxGg0vGfqfiicpdZKMEL5C2VF
lAk2Ntrke1HbpUXT4Y5rDmMpW/DYzh+wuaJutHqWWz79QVyHKETLnCJZ1rMoQ2Sv
zFX5yAtcD5BnoPBo0CE5y/I=
-----END PRIVATE KEY-----)";

    static const char* method_str[] = {
        "POST",
        "PUT",
        "GET",
        "PATCH",
        "DELETE",
    };
    static auto method_str_cnt = int(Method::Method_COUNT_DO_NOT_USE);

    static void fatal(const char* what, int rv)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s: %s", what, nng_strerror(rv));
        throw std::runtime_error(buffer);
    }

    template <typename Type>
    using Deleter = std::unique_ptr<Type, void (*)(Type*)>;

    class RequestImpl : public Request
    {
        nng_http_req* req_;
        const std::string my_uri_;

    public:
        RequestImpl(nng_http_req* req)
            : req_(req), my_uri_(nng_http_req_get_uri(req))
        {
        }

        const std::string& getUri() const override { return my_uri_; }

        const Method getMethod() const override
        {
            auto m = nng_http_req_get_method(req_);
            for (int i = 0; i < method_str_cnt; ++i) {
                if (strcmp(m, method_str[i]) == 0) {
                    return static_cast<Method>(i);
                }
            }
            throw std::runtime_error("non recognized method: " +
                                     std::string(m));
        }

        const std::map<std::string, std::string>& getUriParameters()
            const override
        {
            return uri_parameters_;
        }

        const std::map<std::string, std::string>& getQueries() const override
        {
            return queries_;
        }

        std::string getHeader(const std::string& key) const override
        {
            std::string retval;
            auto header = nng_http_req_get_header(req_, key.c_str());
            if (header != NULL) {
                retval = header;
            }
            return retval;
        }

        std::string getBody() const override { return body_; }

        std::map<std::string, std::string> uri_parameters_;
        std::map<std::string, std::string> queries_;
        std::string body_;
    };  // namespace

    class ResponseImpl : public Response
    {
        nng_http_res* res_;

    public:
        ResponseImpl(nng_http_res* res) : res_(res) {}

        void addHeader(const std::string& key,
                       const std::string& value) override
        {
            int rv = nng_http_res_add_header(res_, key.c_str(), value.c_str());
            if (rv != 0) {
                throw std::runtime_error("Failed to set response header");
            }
        }

        void setBody(const void* data, size_t size) override
        {
            int rv;
            if ((rv = nng_http_res_copy_data(res_, data, size)) != 0) {
                fatal("nng_http_res_copy_data", rv);
            }
        }

        void setBody(const std::string& data) override
        {
            setBody(data.data(), data.size());
        }
    };

    struct RouteTokenImpl : public RouteToken {
        using fn_type = std::function<void(void)>;
        fn_type fn_;
        RouteTokenImpl(fn_type fn) : fn_(fn) {}
        ~RouteTokenImpl() { fn_(); }
    };

    class ServerImpl : public Server,
                       public std::enable_shared_from_this<ServerImpl>
    {
        std::recursive_mutex handler_mutex_;
        nng_http_server* server_{nullptr};
        nng_tls_config* tls_cfg_{nullptr};
        bool started_{false};

        struct route {
            std::regex reg_exp;
            std::vector<std::string> uri_param_key;
            RouteHandler handler;
        };

        struct directory {
            nng_http_handler* handler;
            std::map<std::string, std::string> additional_headers;
        };

        std::map<std::string,           // Method
                 std::map<std::string,  // Base URI
                          std::pair<nng_http_handler*, std::map<int, route>>>>
            routes_;
        std::map<int, directory> directories_;

    public:
        ServerImpl(const std::string& address)
        {
            nng_url* url;
            int rv;
            if ((rv = nng_url_parse(&url, address.c_str())) != 0) {
                fatal("nng_url_parse", rv);
            }
            Deleter<nng_url> free_url(url, nng_url_free);

            const bool secure = (strcmp(url->u_scheme, "https") == 0);
            if (secure) {
#if !SIESTA_ENABLE_TLS
                throw std::logic_error(
                    "SIESTA_ENABLE_TLS must be set to ON for https");
#endif
                if ((rv = nng_tls_config_alloc(&tls_cfg_,
                                               NNG_TLS_MODE_SERVER)) != 0) {
                    fatal("nng_tls_config_alloc", rv);
                }
            }
            // Get a suitable HTTP(S) server instance.  This creates one
            // if it doesn't already exist.
            if ((rv = nng_http_server_hold(&server_, url)) != 0) {
                fatal("nng_http_server_hold", rv);
            }
        }
        ~ServerImpl()
        {
            if (server_ != nullptr) {
                nng_http_server_stop(server_);
                nng_http_server_release(server_);
            }
            if (tls_cfg_ != nullptr) {
                nng_tls_config_free(tls_cfg_);
            }
        }

        void removeRoute(const char* method, const char* base_uri, int id)
        {
            std::lock_guard<std::recursive_mutex> lock(handler_mutex_);
            auto& method_map  = routes_[method];
            auto& handler_map = method_map[base_uri];
            handler_map.second.erase(id);
        }

        void removeDirectory(int id)
        {
            std::lock_guard<std::recursive_mutex> lock(handler_mutex_);
            auto it = directories_.find(id);
            if (it != directories_.end()) {
                nng_http_server_del_handler(server_, it->second.handler);
                nng_http_handler_free(it->second.handler);
                directories_.erase(it);
            }
        }

        std::unique_ptr<RouteToken> addRoute(Method method,
                                             const std::string& uri,
                                             RouteHandler handler) override
        {
            std::lock_guard<std::recursive_mutex> lock(handler_mutex_);
            auto m_str       = method_str[int(method)];
            auto& method_map = routes_[m_str];
            auto base_uri    = uri;
            auto p           = base_uri.find_first_of(".:");
            if (p != std::string::npos) {
                if (p > 1 && base_uri[p - 1] == '/')
                    --p;
                base_uri = base_uri.substr(0, p);
            }
            auto uri_it = method_map.find(base_uri);
            if (uri_it == method_map.end()) {
                nng_http_handler* handler;
                int rv = nng_http_handler_alloc(
                    &handler, base_uri.c_str(), rest_handle);
                if (rv != 0) {
                    fatal("nng_http_handler_alloc", rv);
                }
                if ((p != std::string::npos) &&
                    (rv = nng_http_handler_set_tree(handler)) != 0) {
                    fatal("nng_http_handler_set_tree", rv);
                }
                if ((rv = nng_http_handler_set_data(handler, this, NULL)) !=
                    0) {
                    fatal("nng_http_handler_set_data", rv);
                }
                if ((rv = nng_http_handler_set_method(handler, m_str)) != 0) {
                    fatal("nng_http_handler_set_method", rv);
                }
                // We want to collect the body, and we (arbitrarily) limit this
                // to 128KB.  The default limit is 1MB.  You can explicitly
                // collect the data yourself with another HTTP read transaction
                // by disabling this, but that's a lot of work, especially if
                // you want to handle chunked transfers.
                if ((rv = nng_http_handler_collect_body(
                         handler, true, 1024 * 128)) != 0) {
                    fatal("nng_http_handler_collect_body", rv);
                }
                if ((rv = nng_http_server_add_handler(server_, handler)) != 0) {
                    fatal("nng_http_handler_add_handler", rv);
                }

                method_map[base_uri].first = handler;
                uri_it                     = method_map.find(base_uri);
            }

            auto& handler_map = uri_it->second;
            auto id           = handler_map.second.empty()
                          ? 1
                          : handler_map.second.rbegin()->first + 1;
            auto pThis = shared_from_this();

            route r;
            std::string uri_re = uri;
            // Parse URI parameters (starting with ':')
            const std::regex re_param(":([^/]+)");
            std::smatch m;
            while (std::regex_search(uri_re, m, re_param)) {
                r.uri_param_key.push_back(m[1].str());
                uri_re.replace(m[0].first, m[0].second, "([^/]+)");
            }
            std::regex re(uri_re);
            r.reg_exp = std::move(re);
            r.handler = handler;
            handler_map.second.insert(std::make_pair(id, r));
            return std::unique_ptr<RouteToken>(
                new RouteTokenImpl([pThis, m_str, base_uri, id] {
                    pThis->removeRoute(m_str, base_uri.c_str(), id);
                }));
        }

        std::unique_ptr<RouteToken> addDirectory(
            const std::string& uri,
            const std::string& path) override
        {
            std::lock_guard<std::recursive_mutex> lock(handler_mutex_);
            directory dir;
            int rv = nng_http_handler_alloc_directory(
                &dir.handler, uri.c_str(), path.c_str());
            if (rv != 0) {
                fatal("nng_http_handler_alloc", rv);
            }
            if ((rv = nng_http_handler_set_tree(dir.handler)) != 0) {
                fatal("nng_http_handler_set_tree", rv);
            }
            if ((rv = nng_http_server_add_handler(server_, dir.handler)) != 0) {
                fatal("nng_http_handler_add_handler", rv);
            }
            auto id =
                directories_.empty() ? 1 : directories_.rbegin()->first + 1;
            auto pThis       = shared_from_this();
            directories_[id] = dir;

            return std::unique_ptr<RouteToken>(new RouteTokenImpl(
                [pThis, id] { pThis->removeDirectory(id); }));
        }

        void addCertificate(const std::string& cert,
                            const std::string& key,
                            const std::string& pass) override
        {
            if (tls_cfg_ == nullptr) {
                throw std::logic_error("Server doesn't support TLS");
            }
            if (started_) {
                throw std::runtime_error("Server already started");
            }
            int rv;
            if ((rv = nng_tls_config_own_cert(
                     tls_cfg_,
                     cert.empty() ? tls_cert : cert.c_str(),
                     key.empty() ? tls_key : key.c_str(),
                     pass.empty() ? NULL : pass.c_str())) != 0) {
                fatal("nng_tls_config_own_cert", rv);
            }
        }

        void start() override
        {
            int rv;
            if (started_) {
                throw std::runtime_error("Server already started");
            }
            if (tls_cfg_ != nullptr) {
                // Always add our own self-signed cert for CN=127.0.0.1
                addCertificate(tls_cert, tls_key, "");
                if ((rv = nng_http_server_set_tls(server_, tls_cfg_)) != 0) {
                    fatal("nng_http_server_set_tls", rv);
                }
            }
            if ((rv = nng_http_server_start(server_)) != 0) {
                fatal("nng_http_server_start", rv);
            }
            started_ = true;
        }

        int port() const override
        {
            if (!started_) {
                throw std::runtime_error("Server not started");
            }
            nng_sockaddr addr;
            int rv;
            if ((rv = nng_http_server_get_addr(server_, &addr)) != 0) {
                fatal("nng_http_server_get_port", rv);
            }
            return ntohs(addr.s_in.sa_port);
        }

    private:
        static void rest_handle(nng_aio* aio)
        {
            nng_http_req* req   = (nng_http_req*)nng_aio_get_input(aio, 0);
            nng_http_handler* h = (nng_http_handler*)nng_aio_get_input(aio, 1);
            ServerImpl* pThis   = (ServerImpl*)nng_http_handler_get_data(h);
            nng_http_res* res;
            int rv;

            if ((rv = nng_http_res_alloc(&res)) != 0) {
                nng_aio_finish(aio, rv);
                return;
            }
            nng_http_res_set_data(res, NULL, 0);

            try {
                if (!pThis->handle_rest_request(req, res)) {
                    nng_http_res_set_status(res, NNG_HTTP_STATUS_NOT_FOUND);
                    nng_http_res_set_reason(res, NULL);
                }
            } catch (siesta::Exception& e) {
                nng_http_res_set_status(res, static_cast<uint16_t>(e.status()));
                if (!e.has_reason()) {
                    nng_http_res_set_reason(res, e.what());
                }
            } catch (std::exception& e) {
                nng_http_res_set_status(res,
                                        NNG_HTTP_STATUS_INTERNAL_SERVER_ERROR);
                nng_http_res_set_reason(res, e.what());
            } catch (...) {
                nng_http_res_set_status(res,
                                        NNG_HTTP_STATUS_INTERNAL_SERVER_ERROR);
                nng_http_res_set_reason(res, "Unknown error");
            }
            nng_aio_set_output(aio, 0, res);
            nng_aio_finish(aio, 0);
        }

        bool handle_rest_request(nng_http_req* request, nng_http_res* response)
        {
            const char* method = nng_http_req_get_method(request);
            std::unique_lock<std::recursive_mutex> lock(handler_mutex_);
            auto method_it = routes_.find(method);
            if (method_it != routes_.end()) {
                RequestImpl req(request);
                std::string uri(nng_http_req_get_uri(request));
                auto q_pos = uri.find('?');
                if (q_pos != std::string::npos) {
                    const std::regex r("([^=&]+)=([^=&]+)");
                    std::smatch m;
                    std::string::const_iterator searchStart(uri.cbegin() +
                                                            q_pos + 1);
                    while (std::regex_search(searchStart, uri.cend(), m, r)) {
                        req.queries_.insert(
                            std::make_pair(m[1].str(), m[2].str()));
                        searchStart = m.suffix().first;
                    }
                    uri = uri.substr(0, q_pos);
                }
                auto& uri_map = method_it->second;
                auto uri_it   = uri_map.find(uri);
                if (uri_it == uri_map.end()) {
                    return false;
                }
                auto& handler_map = uri_it->second;
                for (const auto& entry : handler_map.second) {
                    std::smatch m;
                    if (!std::regex_match(uri, m, entry.second.reg_exp)) {
                        continue;
                    }
                    if (m.size() > 1) {
                        if (entry.second.uri_param_key.size() != m.size() - 1) {
                            throw std::runtime_error("Uri parameter error");
                        }
                        for (size_t i = 1; i < m.size(); ++i) {
                            req.uri_parameters_.insert(std::make_pair(
                                entry.second.uri_param_key[i - 1], m[i].str()));
                        }
                    }
                    void* data = nullptr;
                    size_t sz  = 0ULL;
                    nng_http_req_get_data(request, &data, &sz);
                    if (data != nullptr) {
                        req.body_.assign((const char*)data, sz);
                    }
                    lock.unlock();
                    ResponseImpl resp(response);
                    (entry.second.handler)(req, resp);
                    return true;
                }
            }
            return false;
        }
    };  // namespace
}  // namespace

void siesta::server::RouteHolder::operator+=(std::unique_ptr<RouteToken> route)
{
    routes_.push_back(std::move(route));
}

namespace siesta
{
    namespace server
    {
        std::shared_ptr<siesta::server::Server> createServer(
            const std::string& address)
        {
            return std::make_shared<ServerImpl>(address);
        }
    }  // namespace server
}  // namespace siesta
