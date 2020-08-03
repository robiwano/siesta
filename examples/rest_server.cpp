
#include <siesta/server.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

using namespace siesta;

#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>

#define THROW_ON_ERROR(x)                               \
    {                                                   \
        auto rv = x;                                    \
        if (rv != 0) {                                  \
            throw std::runtime_error(nng_strerror(rv)); \
        }                                               \
    }

struct WebsocketConnection : public server::websocket::Reader {
    server::websocket::Writer& writer;
    bool stop_thread{false};
    std::thread thread;
    WebsocketConnection(server::websocket::Writer& w) : writer(w)
    {
        std::cout << "Stream connected (" << this << ")" << std::endl;
    }
    ~WebsocketConnection()
    {
        stopThread();
        std::cout << "Stream disconnected (" << this << ")" << std::endl;
    }
    void stopThread()
    {
        if (thread.joinable()) {
            stop_thread = true;
            thread.join();
            stop_thread = false;
        }
    }
    void onReadData(const std::string& data) override
    {
#if 1
        writer.writeData(data);
#else
        stopThread();
        std::thread t([this, data] {
            using clock = std::chrono::high_resolution_clock;
            auto t_next = clock::now();
            while (!stop_thread) {
                auto t_now = clock::now();
                if (t_now >= t_next) {
                    t_next = t_now + std::chrono::milliseconds(1000);
                    writer.writeData(data);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        thread.swap(t);
#endif
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
        auto server = server::createServer(addr);
        server->start();
        std::cout << "Server started, listening on port " << server->port()
                  << std::endl;

        std::map<std::string, std::string> resource;

        server::TokenHolder h;
        h += server->addRoute(
            HttpMethod::POST,
            "/shutdown",
            [&](const server::rest::Request&, server::rest::Response& res) {
                stop_server = true;
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
            "/api/:name",
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
            "/api/:name",
            [&](const server::rest::Request& req, server::rest::Response& res) {
                auto name = req.getUriParameters().at("name");
                auto it   = resource.find(name);
                if (it == resource.end()) {
                    throw Exception(HttpStatus::NOT_FOUND);
                }
                res.setBody(it->second);
            });

        h += server->addRoute(
            HttpMethod::PUT,
            "/api/:name",
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
            "/api/:name",
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

        h += server->addWebsocket(
            "/test",
            [](server::websocket::Writer& w) {
                return new WebsocketConnection(w);
            },
            2);

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
