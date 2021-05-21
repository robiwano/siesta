#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>

using namespace siesta;

TEST(siesta, server_ok)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

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
        client::postRequest("http://127.0.0.1:8080/my/test/path",
                            req_body,
                            "",
                            std::vector<std::pair<std::string, std::string>>(),
                            5000);

    std::string result;
    EXPECT_NO_THROW(result = f.get());
    EXPECT_EQ(result, req_body);
}

TEST(siesta, server_not_found)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

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
        client::postRequest("http://127.0.0.1:8080/path/not/found",
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

TEST(siesta, server_queries)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

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
    auto f =
        client::postRequest("http://127.0.0.1:8080/my/test/path?foo=23&bar=42",
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

TEST(siesta, server_uri_parameters)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

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
        client::postRequest("http://127.0.0.1:8080/my/23/42",
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
