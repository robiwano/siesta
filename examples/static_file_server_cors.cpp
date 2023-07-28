
#include <siesta/server.h>
using namespace siesta;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

#include <chrono>
#include <iostream>
#include <thread>

#include "ctrl_c_handler.h"

namespace
{
    constexpr auto html =
        R"~(<script type="text/javascript" language="javascript">
var ws = null;

function send()
{
    var URL = "http://" + location.hostname + ":8866/rest/test";  // Cross-domain URL 

    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("GET", URL, false);
    xmlhttp.setRequestHeader("Content-Type", "text/plain");
    xmlhttp.send("");
    document.getElementById("div").innerHTML = xmlhttp.statusText + ":" + xmlhttp.status + "<BR><textarea rows='2' cols='5'>" + xmlhttp.responseText + "</textarea>";

    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("PUT", URL, false);
    xmlhttp.setRequestHeader("Content-Type", "text/plain");
    xmlhttp.send(xmlhttp.responseText);
}

function connect_ws()
{
    var url = "ws://" + location.hostname + ":8866/rest/websocket";
    ws = new WebSocket(url);
    ws.onopen = function () {
        console.log("Connected to websocket!");
    };
    ws.onclose = function () {
        console.log("Disconnected from websocket!");
    };
    ws.onmessage = function (response) {
        console.log(response.data);
    }
}

connect_ws();
</script>
<html>
<body id='bod'><button type="submit" onclick="javascript:send()">call</button>
<div id='div'>
</div></body>
</html>)~";

    struct WebsocketConnection : server::websocket::Reader {
        server::websocket::Writer& writer;
        WebsocketConnection(server::websocket::Writer& w) : writer(w)
        {
            std::cout << "Stream connected (" << this << ")" << std::endl;
            w.send("Hello Websocket Listener!");
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
        static server::websocket::Reader* create(server::websocket::Writer& w)
        {
            return new WebsocketConnection(w);
        }
    };

    struct TempFile {
        fs::path workdir;
        fs::path file;

        TempFile(const std::string& file_name, const void* content, size_t size)
        {
            auto tmp_path = fs::temp_directory_path();
            workdir       = tmp_path / "siesta_serve_directory";
            file          = workdir / fs::path(file_name);
            auto parent   = file.parent_path();
            if (!fs::exists(parent)) {
                fs::create_directories(parent);
            }
            std::ofstream os(file,
                             std::ios_base::trunc | std::ios_base::binary);
            os.write((char*)content, size);
        }
        ~TempFile()
        {
            std::error_code ec;
            fs::remove_all(file.parent_path(), ec);
        }
        std::string path() const
        {
            return fs::relative(file, workdir).generic_string();
        }
        std::string directory() const { return workdir.generic_string(); }
    };

    static TempFile file("index.html", html, strlen(html));

}  // namespace

int main(int argc, char** argv)
{
    ctrlc::set_signal_handler();
    try {
        auto file_server = server::createServer("http://127.0.0.1");
        file_server->start();
        std::cout << "HTTP Server started, listening on port "
                  << file_server->port() << std::endl;

        server::TokenHolder h;
        h += file_server->addDirectory("/", file.directory());

        auto rest_server = server::createServer("http://127.0.0.1:8866");
        rest_server->start();
        std::cout << "RESET Server started, listening on port "
                  << rest_server->port() << std::endl;

        int counter = 1;
        h += rest_server->addRoute(HttpMethod::GET,
                                   "/rest/test",
                                   [&counter](const server::rest::Request&,
                                              server::rest::Response& resp) {
                                       resp.addHeader(
                                           "access-control-allow-origin", "*");
                                       resp.setBody(std::to_string(counter++));
                                   });
        h += rest_server->addRoute(
            HttpMethod::PUT,
            "/rest/test",
            [](const server::rest::Request& req, server::rest::Response& resp) {
                resp.addHeader("access-control-allow-origin", "*");
                std::cout << req.getBody() << std::endl;
            });
        h += rest_server->addRoute(
            HttpMethod::OPTIONS,
            "/rest/test",
            [](const server::rest::Request&, server::rest::Response& resp) {
                resp.addHeader("access-control-allow-methods", "GET,PUT");
                resp.addHeader("access-control-allow-origin", "*");
            });
        h += rest_server->addTextWebsocket("/rest/websocket",
                                           WebsocketConnection::create);
        h += rest_server->addRoute(
            HttpMethod::OPTIONS,
            "/rest/websocket",
            [](const server::rest::Request&, server::rest::Response& resp) {
                resp.addHeader("access-control-allow-methods", "GET,PUT");
                resp.addHeader("access-control-allow-origin", "*");
            });

        while (!ctrlc::signalled()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "Server stopped!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}
