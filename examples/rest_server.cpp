
#include <siesta/server.h>

#include <chrono>
#include <sstream>
#include <thread>

int main(int argc, char** argv)
{
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

    h += server->addRoute(siesta::Method::POST,
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
            retval << "Uri groups are:\n";
            for (auto g : groups) {
                retval << g << std::endl;
            }
            retval << "Body:\n" << req.getBody();
            resp.setBody(retval.str());
        });

    while (!stop_server)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return 0;
}