#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>

using namespace siesta;

namespace
{
    std::string get_address(const std::string& scheme, int port = 0)
    {
        return scheme + "://127.0.0.1:" + std::to_string(port);
    }
}  // namespace

TEST(rest_server, server_ok)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http")));
    const int port = server->port();

    server::TokenHolder TokenHolder;
    EXPECT_NO_THROW(
        TokenHolder += server->addRoute(
            siesta::HttpMethod::POST,
            "/my/test/path",
            [](const server::rest::Request& req, server::rest::Response& resp) {
                //
                resp.setBody(req.getBody());
            }));

    std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    auto f =
        client::postRequest(get_address("http", port) + "/my/test/path",
                            req_body,
                            "",
                            std::vector<std::pair<std::string, std::string>>(),
                            5000);

    std::string result;
    EXPECT_NO_THROW(result = f.get());
    EXPECT_EQ(result, req_body);
}

TEST(rest_server, server_not_found)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http")));
    const int port = server->port();

    server::TokenHolder TokenHolder;
    EXPECT_NO_THROW(
        TokenHolder += server->addRoute(
            siesta::HttpMethod::POST,
            "/my/test/path",
            [](const server::rest::Request& req, server::rest::Response& resp) {
                //
                resp.setBody(req.getBody());
            }));

    std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    auto f =
        client::postRequest(get_address("http", port) + "/path/not/found",
                            req_body,
                            "",
                            std::vector<std::pair<std::string, std::string>>(),
                            5000);

    try {
        auto result = f.get();
        EXPECT_TRUE(false) << "Should not come here!";
    } catch (siesta::Exception& e) {
        EXPECT_EQ(e.status(), siesta::HttpStatus::NOT_FOUND);
    } catch (...) {
        EXPECT_TRUE(false) << "Unknown exception caught!";
    }
}

TEST(rest_server, server_queries)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http")));
    const int port = server->port();

    server::TokenHolder TokenHolder;
    EXPECT_NO_THROW(
        TokenHolder += server->addRoute(
            siesta::HttpMethod::POST,
            "/my/test/path",
            [](const server::rest::Request& req, server::rest::Response& resp) {
                //
                std::stringstream ss;
                ss << req.getQueries().at("foo") << std::endl;
                ss << req.getQueries().at("bar") << std::endl;
                resp.setBody(ss.str());
            }));

    std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    auto f = client::postRequest(
        get_address("http", port) + "/my/test/path?foo=23&bar=42",
        req_body,
        "",
        std::vector<std::pair<std::string, std::string>>(),
        5000);

    try {
        auto result = f.get();
        std::istringstream iss(result);
        char buf[128];
        int expected[] = {23, 42};
        for (int i = 0; iss.getline(buf, sizeof(buf)); ++i) {
            int value;
            int n = sscanf(buf, "%d", &value);
            EXPECT_EQ(n, 1);
            EXPECT_EQ(value, expected[i]);
        }
    } catch (std::exception& e) {
        EXPECT_TRUE(false) << e.what();
    }
}

TEST(rest_server, server_uri_parameters)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http")));
    const int port = server->port();

    server::TokenHolder TokenHolder;
    EXPECT_NO_THROW(
        TokenHolder += server->addRoute(
            siesta::HttpMethod::POST,
            "/my/:test/:path",
            [](const server::rest::Request& req, server::rest::Response& resp) {
                //
                std::stringstream ss;
                ss << req.getUriParameters().at("test") << std::endl;
                ss << req.getUriParameters().at("path") << std::endl;
                resp.setBody(ss.str());
            }));

    std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    auto f =
        client::postRequest(get_address("http", port) + "/my/23/42",
                            req_body,
                            "",
                            std::vector<std::pair<std::string, std::string>>(),
                            5000);

    try {
        auto result = f.get();
        std::istringstream iss(result);
        char buf[128];
        int expected[] = {23, 42};
        for (int i = 0; iss.getline(buf, sizeof(buf)); ++i) {
            int value;
            int n = sscanf(buf, "%d", &value);
            EXPECT_EQ(n, 1);
            EXPECT_EQ(value, expected[i]);
        }
    } catch (std::exception& e) {
        EXPECT_TRUE(false) << e.what();
    }
}

TEST(rest_server, single_file)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http")));
    const int port = server->port();

    constexpr auto text = "foobar";

    constexpr auto route = "/index.html";

    server::TokenHolder TokenHolder;
    EXPECT_NO_THROW(
        TokenHolder += server->addRoute(
            siesta::HttpMethod::GET,
            route,
            [text](const server::rest::Request&, server::rest::Response& resp) {
                resp.setBody(text);
            }));

    auto f = client::getRequest(get_address("http", port) + route);

    try {
        auto result = f.get();
        EXPECT_EQ(result, text);
    } catch (std::exception& e) {
        EXPECT_TRUE(false) << e.what();
    }
}
