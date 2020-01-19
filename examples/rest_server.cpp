
#include <siesta/server.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        std::string addr = "http://127.0.0.1:9080";
        if (argc > 1) {
            addr = argv[1];
        }
        auto server = server::createServer(addr);
        server->start();
        std::cout << "Server started, listening on port " << server->port()
                  << std::endl;

        bool stop_server = false;

        std::map<std::string, std::string> resource;

        server::RouteHolder h;
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
                    throw Exception(HttpStatus::CONFLICT,
                                    "Element '" + name + "' already exists");
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
                    throw Exception(HttpStatus::NOT_FOUND);
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
                    throw Exception(HttpStatus::NOT_FOUND);
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
                    throw Exception(HttpStatus::NOT_FOUND);
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
        return -1;
    }

    return 0;
}