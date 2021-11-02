# Siesta

- [Siesta](#siesta)
- [Features](#features)
  - [REST API](#rest-api)
    - [URI parameters](#uri-parameters)
    - [Queries](#queries)
  - [Websockets](#websockets)
- [Building](#building)
  - [Requirements](#requirements)
  - [Quick start](#quick-start)
- [Examples](#examples)
  - [Hello World (REST API server)](#hello-world-rest-api-server)
  - [Hello World (client)](#hello-world-client)
  - [Serving a static directory](#serving-a-static-directory)
  - [Websocket server](#websocket-server)
  - [Websocket client](#websocket-client)
  
Siesta is a minimalistic HTTP, REST and Websocket framework for C++, written in pure-C++11, based upon NNG ([Nanomsg-Next-Generation](https://nng.nanomsg.org/)).

Main features:
- Webserver
- REST API
- Websocket (binary/text)

The design goals for Siesta are:

- Minimalistic and simple interface.
- Dynamic route creation and removal.
- Minimal dependencies. Unless you need TLS, the **only** dependency is NNG.
- No required payload format. Requests and responses are plain strings/data.
- Cross platform.

Siesta will basically run on any platform supported by NNG. These are (taken from NNG Github Readme): Linux, macOS, Windows (Vista or better), illumos, Solaris, FreeBSD, Android, and iOS.

Since payloads are plain strings/data, you're free to use any layer on top of this, f.i. JSON or XML.

# Features

## REST API

### URI parameters

By prefixing a URI route segment with ':', that segment of the URI becomes a *URI parameter*, which can be retrieved in the route handler:
```cpp
...
server::TokenHolder h;
h += server->addRoute(
            HttpMethod::GET,
            "/:resource/:index",
            [](const server::rest::Request& req, server::rest::Response& resp) {
                const auto& uri_params = req.getUriParameters();
                std::stringstream body;
                body << "resource=" << uri_params.at("resource") << std::endl;
                body << "index=" << uri_params.at("index") << std::endl;
                resp.setBody(body.str());
            });
...
```

### Queries

URI Queries are supported via `server::Request::getQueries` method:
```cpp
...
server::TokenHolder h;
h += server->addRoute(
            HttpMethod::GET,
            "/resource/get",
            [](const server::rest::Request& req, server::rest::Response& resp) {
                const auto& queries = req.getQueries();
                std::stringstream body;
                body << "Queries:" << std::endl;
                for (auto q : queries) {
                    body << q.first << "=" << q.second << std::endl;
                }
                resp.setBody(body.str());
            });
...
```

## Websockets

The websocket API is built upon a factory pattern, where the websocket session is implemented by the user of the **siesta** framework, see [example below](#websocket-server).

# Building

## Requirements

You will need a compiler supporting C++11 and C99, and [CMake](https://cmake.org/) version 3.11 or newer. [Ninja](https://ninja-build.org/) is the recommended build system.

If you need TLS support, set the SIESTA_ENABLE_TLS CMake variable to ON. This will make CMake download [ARM mbedTLS](https://tls.mbed.org/) upon configuration and enable TLS support for NNG.

**Note:** When using ARM mbedTLS, the license will change to [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0).

## Quick start

To build in a Linux environment:

    $ mkdir build
    $ cd build
    $ cmake -GNinja ..
    $ ninja
    $ ninja test

# Examples

## Hello World (REST API server)

```cpp
#include <siesta/server.h>

#include <chrono>
#include <iostream>
#include <thread>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        // Use "https://...." for a secure server
        auto server = server::createServer("http://127.0.0.1:9080");
        server->start();
        std::cout << "Server started, listening on port " << server->port()
                  << std::endl;

        server::TokenHolder h;

        // Must hold on to the returned Token, otherwise the route will
        // be removed when the Token is destroyed.
        h += server->addRoute(
            HttpMethod::GET,
            "/",
            [](const server::rest::Request&, server::rest::Response& resp) {
                resp.setBody("Hello, World!");
            });

        // Run the server forever
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
```

## Hello World (client)

```cpp
#include <siesta/client.h>

#include <chrono>
#include <iostream>
#include <thread>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        auto f        = client::getRequest("http://127.0.0.1:9080/");
        auto response = f.get();
        std::cout << response << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}
```

## Serving a static directory
```cpp
#include <siesta/server.h>
using namespace siesta;

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv)
{
    try {
        if (argc < 2) {
            std::cout << "Options: [address] base_path" << std::endl;
            return 1;
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

        server::TokenHolder h;
        h += server->addDirectory("/", base_path);

        std::cout << "Serving folder '" << base_path << "' under URI '/'" << std::endl;

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}
```

## Websocket server

```cpp
#include <siesta/server.h>
using namespace siesta;

#include <chrono>
#include <iostream>
#include <thread>

struct WebsocketConnection : server::websocket::Reader {
    server::websocket::Writer& writer;
    WebsocketConnection(server::websocket::Writer& w) : writer(w)
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
        std::cout << "Echoing data '" << data << "' (" << this << ")" << std::endl;
        writer.send(data);
    }

    // The websocket factory method
    static server::websocket::Reader* create(server::websocket::Writer& w)
    {
        return new WebsocketConnection(w);
    }
};

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

        auto token = server->addWebsocket("/test", WebsocketConnection::create);

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}
```

## Websocket client

```cpp
#include <siesta/client.h>
using namespace siesta;

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv)
{
    try {
        std::string addr = "ws://127.0.0.1:9080/test";
        if (argc > 1) {
            addr = argv[1];
        }
        auto on_open = [](client::websocket::Writer&) {
            std::cout << "Client connected!" << std::endl;
        };
        auto on_close = [](client::websocket::Writer&,
                           const std::string& error) {
            std::cout << "Error: " << error << std::endl;
        };
        auto on_close = [](client::websocket::Writer&) {
            std::cout << "Client disconnected!" << std::endl;
        };

        auto on_message = [](client::websocket::Writer&,
                             const std::string& data) {
            std::cout << "Received: " << data << std::endl;
        };
        auto client = client::websocket::connect(addr,
            on_message,
            on_open,
            on_error,
            on_close);

        while (true) {
            std::string input;
            std::getline(std::cin, input);
            client->send(input);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```
