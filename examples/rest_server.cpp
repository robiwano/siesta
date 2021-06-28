
#include <siesta/server.h>
using namespace siesta;

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include "ctrl_c_handler.h"

int main(int argc, char** argv)
{
    ctrlc::set_signal_handler();
    try {
        bool rest_shutdown = false;
        std::string addr   = "http://127.0.0.1:9080";
        if (argc > 1) {
            addr = argv[1];
        }
        auto server = server::createServer(addr);
        server->start();
        std::cout << "Server started, listening on port " << server->port()
                  << std::endl;

        std::map<std::string, std::string> resource;

        server::TokenHolder h;
        h += server->addRoute(
            HttpMethod::POST,
            "/shutdown",
            [&](const server::rest::Request& req, server::rest::Response& res) {
                if (req.getHeader("api_key").find("123456") == std::string::npos) { // make your own system
                    res.setBody("{\"error\":\"Invalid Api Key\"}"); // no json library
                   return;
                }
                rest_shutdown = true;
                res.setBody("OK");
            });

        h += server->addRoute(
            HttpMethod::GET,
            "/api",
            [&](const server::rest::Request& req, server::rest::Response& res) {
                std::stringstream body;
                for (auto it : resource) {
                    body << req.getUri() << "/" << it.first << std::endl;
                }
                res.setBody(body.str());
            });

        h += server->addRoute(
            HttpMethod::POST,
            "/api/create/:name",
            [&](const server::rest::Request& req, server::rest::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it != resource.end()) {
                    throw Exception(HttpStatus::CONFLICT,
                                    "Element '" + name + "' already exists");
                }
                resource[name] = req.getBody();
            });

        h += server->addRoute(
            HttpMethod::GET,
            "/api/get/:name",
            [&](const server::rest::Request& req, server::rest::Response& res) {

                if (req.getHeader("api_key").find("123456") == std::string::npos) { // make your own system
                    res.setBody("{\"error\":\"Invalid Api Key\"}"); // no json library
                    return; // i prefer over throwing an exception...
                }
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    throw Exception(HttpStatus::NOT_FOUND);
                }
                res.setBody(it->second);
            });

        h += server->addRoute(
            HttpMethod::PUT,
            "/api/put/:name",
            [&](const server::rest::Request& req, server::rest::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    throw Exception(HttpStatus::NOT_FOUND);
                }
                it->second = req.getBody();
            });

        h += server->addRoute(
            HttpMethod::DEL,
            "/api/del/:name",
            [&](const server::rest::Request& req, server::rest::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    throw Exception(HttpStatus::NOT_FOUND);
                }
                resource.erase(it);
            });

        h += server->addRoute(
            HttpMethod::PATCH,
            "/api/:name/:index",
            [](const server::rest::Request& req, server::rest::Response& resp) {
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

        while (!ctrlc::signalled() && !rest_shutdown) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "Server stopped!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
