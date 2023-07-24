
#include <siesta/server.h>
using namespace siesta;

#include <chrono>
#include <iostream>
#include <thread>

#include "ctrl_c_handler.h"

// Dummy struct to show how to supply a websocket connection with an "owner"
struct WebsocketData {
};

struct WebsocketConnection : server::websocket::Reader {
    WebsocketData& owner;
    server::websocket::Writer& writer;
    WebsocketConnection(WebsocketData& owner, server::websocket::Writer& w)
        : owner(owner), writer(w)
    {
        std::cout << "Stream connected (" << this << ")" << std::endl;
    }
    ~WebsocketConnection()
    {
        std::cout << "Stream disconnected (" << this << ")" << std::endl;
    }
    void onMessage(const std::string& data) override
    {
        // Just echo back received data
        std::cout << "Echoing back '" << data << "' (" << this << ")"
                  << std::endl;
        writer.send(data);
    }

    // The websocket factory method
    static server::websocket::Reader* create(WebsocketData& owner,
                                             server::websocket::Writer& w)
    {
        return new WebsocketConnection(owner, w);
    }
};

int main(int argc, char** argv)
{
    ctrlc::set_signal_handler();
    try {
        std::string addr = "http://127.0.0.1:9080";
        if (argc > 1) {
            addr = argv[1];
        }
        {
            auto server = server::createServer(addr);
            server->start();
            std::cout << "Server started, listening on port " << server->port()
                      << std::endl;

            WebsocketData my_data;
            auto token =
                server->addTextWebsocket("/test",
                                         std::bind(&WebsocketConnection::create,
                                                   my_data,
                                                   std::placeholders::_1));

            while (!ctrlc::signalled()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        std::cout << "Server stopped!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
