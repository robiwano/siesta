#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/util/platform.h>
#include <siesta/server.h>

#include <mutex>
#include <regex>
#include <stdexcept>

using namespace siesta;
using namespace siesta::server;

namespace
{
    static const char* method_str[] = {
        "POST",
        "PUT",
        "GET",
        "PATCH",
        "DELETE",
    };
    static auto method_str_cnt = int(Method::Method_COUNT_DO_NOT_USE);

    // utility function
    void fatal(const char* what, int rv)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s: %s", what, nng_strerror(rv));
        throw std::runtime_error(buffer);
    }

    template <class _Mutex>
    class unlockable_lock_guard
    {  // class with destructor that unlocks a mutex
    public:
        using mutex_type = _Mutex;

        explicit unlockable_lock_guard(_Mutex& _Mtx) : _MyMutex(_Mtx)
        {  // construct and lock
            _MyMutex.lock();
        }

        ~unlockable_lock_guard() noexcept
        {
            if (_do_unlock)
                _MyMutex.unlock();
        }

        void unlock()
        {
            _do_unlock = false;
            _MyMutex.unlock();
        }

        unlockable_lock_guard(const unlockable_lock_guard&) = delete;
        unlockable_lock_guard& operator=(const unlockable_lock_guard&) = delete;

    private:
        bool _do_unlock{true};
        _Mutex& _MyMutex;
    };

    class RequestImpl : public Request
    {
        nng_http_req* req_;

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

        const std::string my_uri_;
        std::map<std::string, std::string> uri_parameters_;
        std::map<std::string, std::string> queries_;
        std::string body_;
    };  // namespace

    class ResponseImpl : public Response
    {
        nng_http_res* res_;

    public:
        ResponseImpl(nng_http_res* res) : res_(res) {}
        void setHttpStatus(
            siesta::HttpStatus status,
            const std::string& optional_reason /* = "" */) override
        {
            status_ = status;
            reason_ = optional_reason;
        }

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
            body_.assign((const char*)data, size);
        }

        void setBody(const std::string& data) override { body_ = data; }

        std::string body_;
        siesta::HttpStatus status_{siesta::HttpStatus::OK};
        std::string reason_;
    };

    struct RouteImpl : public Route {
        using fn_type = std::function<void(void)>;
        fn_type fn_;
        RouteImpl(fn_type fn) : fn_(fn) {}
        ~RouteImpl() { fn_(); }
    };

    class ServerImpl : public Server,
                       public std::enable_shared_from_this<ServerImpl>
    {
        std::mutex handler_mutex_;
        nng_http_server* server_;

        struct route {
            std::regex reg_exp;
            std::vector<std::string> uri_param_key;
            RouteHandler handler;
        };

        std::map<std::string,
                 std::pair<nng_http_handler*, std::map<int, route>>>
            routes_;

    public:
        ServerImpl(const std::string& ip_address, const int port)
            : server_(nullptr)
        {
            char rest_addr[128];
            nng_url* url;
            int rv;
            // Set up some strings, etc.  We use the port number
            // from the argument list.
            snprintf(rest_addr,
                     sizeof(rest_addr),
                     "http://%s:%d",
                     ip_address.c_str(),
                     port);
            if ((rv = nng_url_parse(&url, rest_addr)) != 0) {
                fatal("nng_url_parse", rv);
            }
            std::unique_ptr<nng_url, void (*)(nng_url*)> free_url(url,
                                                                  nng_url_free);
            // Get a suitable HTTP server instance.  This creates one
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
        }

        void removeRoute(const char* method, int id)
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            auto& handler_map = routes_[method];
            handler_map.second.erase(id);
        }

        std::unique_ptr<Route> addRoute(Method method,
                                        const std::string& uri,
                                        RouteHandler handler) override
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            auto m_str    = method_str[int(method)];
            auto route_it = routes_.find(m_str);
            if (route_it == routes_.end()) {
                nng_http_handler* handler;
                int rv = nng_http_handler_alloc(&handler, NULL, rest_handle);
                if (rv != 0) {
                    fatal("nng_http_handler_alloc", rv);
                }
                if ((rv = nng_http_handler_set_tree(handler)) != 0) {
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
                routes_[m_str].first = handler;
                route_it             = routes_.find(m_str);
            }

            auto& handler_map = route_it->second;
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
            return std::unique_ptr<Route>(new RouteImpl(
                [pThis, m_str, id] { pThis->removeRoute(m_str, id); }));
        }

        void start() override
        {
            int rv;
            if ((rv = nng_http_server_start(server_)) != 0) {
                fatal("nng_http_server_start", rv);
            }
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
            unlockable_lock_guard<std::mutex> lock(handler_mutex_);
            auto route_it = routes_.find(method);
            if (route_it != routes_.end()) {
                RequestImpl req(request);
                void* data = NULL;
                size_t sz  = 0ULL;
                nng_http_req_get_data(request, &data, &sz);
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
                auto& handler_map = route_it->second;
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
                    if (data != nullptr) {
                        req.body_.assign((const char*)data, sz);
                    }
                    ResponseImpl resp(response);
                    auto h = entry.second.handler;
                    lock.unlock();
                    h(req, resp);
                    if (!resp.body_.empty()) {
                        int rv = nng_http_res_copy_data(
                            response, resp.body_.data(), resp.body_.length());
                        if (rv != 0) {
                            throw std::runtime_error(
                                "Failed to copy data to response");
                        }
                    }
                    if (resp.status_ != siesta::HttpStatus::OK) {
                        nng_http_res_set_status(response,
                                                uint16_t(resp.status_));
                        if (!resp.reason_.empty()) {
                            nng_http_res_set_reason(response,
                                                    resp.reason_.c_str());
                        }
                    }
                    return true;
                }
            }
            return false;
        }
    };
}  // namespace

siesta::server::RouteHolder& siesta::server::RouteHolder::operator+=(
    std::unique_ptr<Route> route)
{
    routes_.push_back(std::move(route));
    return *this;
}

namespace siesta
{
    namespace server
    {
        std::shared_ptr<Server> createServer(const std::string& ip_address,
                                             const int port)
        {
            return std::make_shared<ServerImpl>(ip_address, port);
        }
    }  // namespace server
}  // namespace siesta
