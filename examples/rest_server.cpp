
#include <siesta/server.h>

#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
    auto server = siesta::createServer("127.0.0.1", 8088);

    siesta::RouteHolder h;
    h << server->addRoute(
        siesta::Method::GET, "/api/rest/test", [](const siesta::Request& r) {
            //
            return "Hello World!";
        });

    server->start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}