# Siesta

Siesta is a minimalistic HTTP and REST framework for C++, written in pure-C++11, based upon NNG ([Nanomsg-Next-Generation](https://nng.nanomsg.org/)).

The design goals for Siesta are:

- Minimalistic and simple interface.
- Dynamic route creation and removal.
- Minimal dependencies. Unless you need TLS, the **only** dependency is NNG.
- No required payload format. Requests and responses are plain strings/data.
- Cross platform.

Siesta will basically run on any platform supported by NNG. These are (taken from NNG Github Readme): Linux, macOS, Windows (Vista or better), illumos, Solaris, FreeBSD, Android, and iOS.

Since payloads are plain strings/data, you're free to use any layer on top of this, f.i. JSON or XML.

# Features

## URI parameters

By prefixing a URI route segment with ':', that segment of the URI becomes a *URI parameter*, which can be retrieved in the route handler:
```cpp
...
server::RouteHolder h;
h += server->addRoute(
            Method::GET,
            "/:resource/:index",
            [](const server::Request& req, server::Response& resp) {
                const auto& uri_params = req.getUriParameters();
                std::stringstream body;
                body << "resource=" << uri_params.at("resource") << std::endl;
                body << "index=" << uri_params.at("index") << std::endl;
                resp.setBody(body.str());
            });
...
```

## Queries

URI Queries are supported via `server::Request::getQueries` method:
```cpp
...
server::RouteHolder h;
h += server->addRoute(
            Method::GET,
            "/resource/get",
            [](const server::Request& req, server::Response& resp) {
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

# Example

## Hello World (server)

```cpp
#include <siesta/server.h>
#include <iostream>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        // Use "https://...." for a secure server
        auto server = server::createServer("http://127.0.0.1:9080");
        server->start();
        std::cout << "Server started, listening on port " << server->port()
                  << std::endl;

        server::RouteHolder h;

		// Must hold on to the returned RouteToken, otherwise the route will
        // be removed when the RouteToken is destroyed.
        h += server->addRoute(
            Method::GET,
            "/",
            [](const server::Request&, server::Response& resp) {
                resp.setBody("Hello, World!");
            });

        // Run the server forever
        while (true)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
```

## Hello World (client)

```cpp
#include <siesta/client.h>
#include <iostream>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        auto f        = client::getRequest("http://127.0.0.1:9080/", 1000);
        auto response = f.get();
        std::cout << response << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
```