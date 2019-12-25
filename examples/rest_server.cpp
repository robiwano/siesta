
#include <siesta/server.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

int main(int argc, char** argv)
{
    try {
        auto server = siesta::createServer("127.0.0.1", 8088);
        server->start();

        bool stop_server = false;

        std::map<std::string, std::string> resource;

        siesta::RouteHolder h;
        h += server->addRoute(
            siesta::Method::POST,
            "/shutdown",
            [&](const siesta::Request&, siesta::Response& res) {
                stop_server = true;
                res.setBody("OK");
            });

        h += server->addRoute(
            siesta::Method::GET,
            "/api",
            [&](const siesta::Request& req, siesta::Response& res) {
                std::stringstream body;
                for (auto it : resource) {
                    body << req.getUri() << "/" << it.first << std::endl;
                }
                res.setBody(body.str());
            });

        h += server->addRoute(
            siesta::Method::POST,
            "/api/:name",
            [&](const siesta::Request& req, siesta::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it != resource.end()) {
                    res.setHttpStatus(siesta::HttpStatus::CONFLICT, "Element '" + name + "' already exists");
                    return;
                }
                resource[name] = req.getBody();
            });

        h += server->addRoute(
            siesta::Method::GET,
            "/api/:name",
            [&](const siesta::Request& req, siesta::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    res.setHttpStatus(siesta::HttpStatus::NOT_FOUND);
                    return;
                }
                res.setBody(it->second);
            });

        h += server->addRoute(
            siesta::Method::PUT,
            "/api/:name",
            [&](const siesta::Request& req, siesta::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    res.setHttpStatus(siesta::HttpStatus::NOT_FOUND);
                    return;
                }
                it->second = req.getBody();
            });

        h += server->addRoute(
            siesta::Method::DELETE,
            "/api/:name",
            [&](const siesta::Request& req, siesta::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    res.setHttpStatus(siesta::HttpStatus::NOT_FOUND);
                    return;
                }
                resource.erase(it);
            });

        h += server->addRoute(
            siesta::Method::PATCH,
            "/api/:name/:index",
            [](const siesta::Request& req, siesta::Response& resp) {
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
                resp.setBody(retval.str());
            });

        while (!stop_server)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}