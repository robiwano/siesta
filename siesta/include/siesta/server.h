#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace siesta
{
    class Route
    {
    public:
        virtual ~Route() = default;
    };

    class RouteHolder
    {
        std::vector<std::unique_ptr<Route>> routes_;

    public:
        RouteHolder& operator+=(std::unique_ptr<Route> route);
    };

    enum class Method {
        POST,
        PUT,
        GET,
        PATCH,
        DELETE,
    };

    class Request
    {
    public:
        virtual ~Request()                                           = default;
        virtual const std::vector<std::string>& getUriGroups() const = 0;
        virtual std::string getHeader(const std::string& key) const  = 0;
        virtual std::string getBody() const                          = 0;
    };

    using RouteHandler = std::function<std::string(const Request&)>;

    class Server
    {
    public:
        virtual ~Server()                                             = default;
        virtual std::unique_ptr<Route> addRoute(Method method,
                                                const std::string& uri_regexp,
                                                RouteHandler handler) = 0;
        virtual void start()                                          = 0;
    };

    std::shared_ptr<Server> createServer(const std::string& ip_address,
                                         const int port);

    // TODO:
    // std::shared_ptr<Server> createSecureServer(const std::string& ip_address,
    // const int port);
}  // namespace siesta
