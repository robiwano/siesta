
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
function send()
{
    var URL = "http://" + location.host + "/rest/test";  //Your URL

    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("GET", URL, false);
    xmlhttp.setRequestHeader("Content-Type", "text/plain");
    xmlhttp.send("");
    document.getElementById("div").innerHTML = xmlhttp.statusText + ":" + xmlhttp.status + "<BR><textarea rows='2' cols='5'>" + xmlhttp.responseText + "</textarea>";
}
</script>
<html>
<body id='bod'><button type="submit" onclick="javascript:send()">call</button>
<div id='div'>
</div></body>
</html>)~";

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
        auto server = server::createServer("http://127.0.0.1");
        server->start();
        std::cout << "HTTP Server started, listening on port " << server->port()
                  << std::endl;

        server::TokenHolder h;
        h += server->addDirectory("/", file.directory());

        int counter = 1;
        h += server->addRoute(HttpMethod::GET,
                              "/rest/test",
                              [&counter](const server::rest::Request&,
                                         server::rest::Response& resp) {
                                  resp.setBody(std::to_string(counter++));
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
