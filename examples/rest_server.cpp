
#include <siesta/server.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        int port = 0;
        if (argc > 1) {
            port = atoi(argv[1]);
        }
        auto server = server::createServer("127.0.0.1", port);
        server->start();
        std::cout << "Server started, listening on port " << server->port()
                  << std::endl;

        bool stop_server = false;

        std::map<std::string, std::string> resource;

        server::RouteHolder h;
        h += server->addRoute(
            Method::GET,
            "/",
            [](const server::Request& req, server::Response& res) {
                res.setBody("Hello, World!");
            });

        h += server->addRoute(
            Method::POST,
            "/shutdown",
            [&](const server::Request&, server::Response& res) {
                stop_server = true;
                res.setBody("OK");
            });

        h += server->addRoute(
            Method::GET,
            "/api",
            [&](const server::Request& req, server::Response& res) {
                std::stringstream body;
                for (auto it : resource) {
                    body << req.getUri() << "/" << it.first << std::endl;
                }
                res.setBody(body.str());
            });

        h += server->addRoute(
            Method::POST,
            "/api/:name",
            [&](const server::Request& req, server::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it != resource.end()) {
                    res.setHttpStatus(HttpStatus::CONFLICT,
                                      "Element '" + name + "' already exists");
                    return;
                }
                resource[name] = req.getBody();
            });

        h += server->addRoute(
            Method::GET,
            "/api/:name",
            [&](const server::Request& req, server::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    res.setHttpStatus(HttpStatus::NOT_FOUND);
                    return;
                }
                res.setBody(it->second);
            });

        h += server->addRoute(
            Method::PUT,
            "/api/:name",
            [&](const server::Request& req, server::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    res.setHttpStatus(HttpStatus::NOT_FOUND);
                    return;
                }
                it->second = req.getBody();
            });

        h += server->addRoute(
            Method::DELETE,
            "/api/:name",
            [&](const server::Request& req, server::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    res.setHttpStatus(HttpStatus::NOT_FOUND);
                    return;
                }
                resource.erase(it);
            });

        h += server->addRoute(
            Method::PATCH,
            "/api/:name/:index",
            [](const server::Request& req, server::Response& resp) {
                //
                const auto& params = req.getUriParameters();
                std::stringstream retval;
                retval << "Uri parameters:" << std::endl;
                for (auto p : params) {
                    retval << p.first << "=" << p.second << std::endl;
                }
                const auto& queries = req.getQueries();
                retval << "Queries:" << std::endl;
                for (auto q : queries) {
                    retval << q.first << "=" << q.second << std::endl;
                }
                retval << "Body:" << std::endl;
                retval << req.getBody() << std::endl;
                resp.setBody(retval.str());
            });

        while (!stop_server)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::cout << "Server stopped!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}