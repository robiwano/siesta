
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

        siesta::RouteHolder h;
        h += server->addRoute(
            siesta::Method::GET,
            "/barf",
            [](const siesta::Request&, siesta::Response&) {
                throw std::runtime_error("This will end up as error message");
            });

        h += server->addRoute(
            siesta::Method::POST,
            "/shutdown",
            [&](const siesta::Request&, siesta::Response& req) {
                stop_server = true;
                req.setBody("OK");
            });

        h += server->addRoute(
            siesta::Method::PUT,
            "/api/([a-z]+)/test",
            [](const siesta::Request& req, siesta::Response& resp) {
                //
                auto groups = req.getUriGroups();
                std::stringstream retval;
                retval << "Uri groups are:" << std::endl;
                for (auto g : groups) {
                    retval << g << std::endl;
                }
                retval << "Body:" << std::endl << req.getBody();
                resp.setBody(retval.str());
            });

        h += server->addRoute(
            siesta::Method::GET,
            "/api/([a-z]+)/get",
            [](const siesta::Request& req, siesta::Response& resp) {
                //
                auto groups = req.getUriGroups();
                std::stringstream retval;
                retval << "Uri groups are:" << std::endl;
                for (auto g : groups) {
                    retval << g << std::endl;
                }
                auto queries = req.getQueries();
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