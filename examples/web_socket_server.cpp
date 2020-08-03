
#include <siesta/server.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>

#define THROW_ON_ERROR(x)                               \
    {                                                   \
        auto rv = x;                                    \
        if (rv != 0) {                                  \
            throw std::runtime_error(nng_strerror(rv)); \
        }                                               \
    }

struct WebsocketConnection : public siesta::server::websocket::Reader {
    siesta::server::websocket::Writer& writer;
    WebsocketConnection(siesta::server::websocket::Writer& w) : writer(w)
    {
        std::cout << "Stream connected (" << this << ")" << std::endl;
    }
    ~WebsocketConnection()
    {
        std::cout << "Stream disconnected (" << this << ")" << std::endl;
    }
    void onReadData(const std::string& data) override
    {
        // Just echo back received data
        writer.writeData(data);
    }
};

void set_signal_handler();
static bool stop_server = false;

int main(int argc, char** argv)
{
    set_signal_handler();
    try {
        std::string addr = "http://127.0.0.1:9080";
        if (argc > 1) {
            addr = argv[1];
        }
        auto server = siesta::server::createServer(addr);
        server->start();
        std::cout << "Server started, listening on port " << server->port()
                  << std::endl;

        auto token = server->addWebsocket("/test",
                                  [](siesta::server::websocket::Writer& w) {
                                      return new WebsocketConnection(w);
                                  });

        while (!stop_server) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "Server stopped!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}

#ifdef _WIN32

#include <objbase.h>
#include <windows.h>

BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType)
{
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
        stop_server = true;
        return TRUE;
    default:
        // Pass signal on to the next handler
        return FALSE;
    }
}

void set_signal_handler() { SetConsoleCtrlHandler(HandlerRoutine, TRUE); }

#else
#include <signal.h>

void intHandler(int signal)
{
    (void)signal;
    stop_server = true;
}

void set_signal_handler()
{
    signal(SIGINT, intHandler);
    signal(SIGSTOP, intHandler);
    signal(SIGTERM, intHandler);
}

#endif
