#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>

using namespace siesta;

TEST(siesta, server_ok)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

    server::RouteHolder routeHolder;
    EXPECT_NO_THROW(routeHolder += server->addRoute(
                        siesta::Method::POST,
                        "/my/test/path",
                        [](const server::Request& req, server::Response& resp) {
                            //
                            resp.setBody(req.getBody());
                        }));

    std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    auto f = client::postRequest(
        "http://127.0.0.1:8080/my/test/path", req_body, "", 5000);

    std::string result;
    EXPECT_NO_THROW(result = f.get());
    EXPECT_EQ(result, req_body);
}

TEST(siesta, server_not_found)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

    server::RouteHolder routeHolder;
    EXPECT_NO_THROW(routeHolder += server->addRoute(
                        siesta::Method::POST,
                        "/my/test/path",
                        [](const server::Request& req, server::Response& resp) {
                            //
                            resp.setBody(req.getBody());
                        }));

    std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    auto f = client::postRequest(
        "http://127.0.0.1:8080/path/not/found", req_body, "", 5000);

    try {
        auto result = f.get();
        EXPECT_TRUE(false) << "Should not come here!";
    } catch (siesta::client::ClientException& e) {
        EXPECT_EQ(e.status(), siesta::HttpStatus::NOT_FOUND);
    } catch (...) {
        EXPECT_TRUE(false) << "Unknown exception caught!";
    }
}
