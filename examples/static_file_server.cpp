
#include <siesta/server.h>
using namespace siesta;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

#include <chrono>
#include <iostream>
#include <thread>

namespace
{
    constexpr auto html =
        R"~(<script type="text/javascript" language="javascript">

function send()
{
    var URL = "http://" + location.host + ":8080/rest/test";  //Your URL

    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = callbackFunction(xmlhttp);
    xmlhttp.open("GET", URL, false);
    xmlhttp.setRequestHeader("Content-Type", "text/plain");
    xmlhttp.onreadystatechange = callbackFunction(xmlhttp);
    xmlhttp.send("");
    document.getElementById("div").innerHTML = xmlhttp.statusText + ":" + xmlhttp.status + "<BR><textarea rows='2' cols='5'>" + xmlhttp.responseText + "</textarea>";
}

function callbackFunction(xmlhttp) 
{
    //alert(xmlhttp.responseXML);
}
</script>


<html>
<body id='bod'><button type="submit" onclick="javascript:send()">call</button>
<div id='div'>
</div></body>
</html>")~";

    struct TempFile {
        fs::path workdir;
        fs::path file;

        TempFile(const std::string& file_name, const void* content, size_t size)
        {
            auto tmp_path = fs::temp_directory_path();
            workdir       = tmp_path / "serve_directory";
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
            while (!fs::equivalent(file, fs::temp_directory_path())) {
                fs::remove(file);
                file = file.parent_path();
            }
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
    try {
        auto server = server::createServer("http://127.0.0.1");
        server->start();
        std::cout << "HTTP Server started, listening on port " << server->port()
                  << std::endl;

        server::RouteHolder h;
        h += server->addDirectory("/", file.directory());

        auto rest_server = server::createServer("http://127.0.0.1:8080");
        rest_server->start();
        std::cout << "REST Server started, listening on port "
                  << rest_server->port() << std::endl;

        int counter = 1;
        h += rest_server->addRoute(
            Method::GET,
            "/rest/test",
            [&counter](const server::Request& req, server::Response& resp) {
                resp.setBody(std::to_string(counter++));
            });

        while (true)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}
