
#include <siesta/server.h>

#include <chrono>
#include <iostream>
#include <thread>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        if (argc == 1) {
            std::cout << "Options: [address] base_path" << std::endl;
            return 0;
        }
        std::string addr      = "http://127.0.0.1";
        const char* base_path = argv[1];
        if (argc > 2) {
            addr      = argv[1];
            base_path = argv[2];
        }
        auto server = server::createServer(addr);
        server->start();
        std::cout << "Server started, listening on port " << server->port()
                  << std::endl;

        server::RouteHolder h;
        h += server->addDirectory("/", base_path);

        std::cout << "Serving folder '" << base_path << "' for URI '/'"
                  << std::endl;

        while (true)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}
