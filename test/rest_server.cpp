#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>

using namespace siesta;

TEST(siesta, server_create)
{
    auto server = server::createServer("127.0.0.1", 8080);
    server->start();

    server::RouteHolder routeHolder;
    routeHolder += server->addRoute(
        siesta::Method::POST,
        "/my/test/path",
        [](const server::Request& req, server::Response& resp) {
            //
            resp.setBody(req.getBody());
        });

    std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    auto f = client::postRequest(
        "http://127.0.0.1:8080/my/test/path", req_body, "", 5000);

    auto result = f.get()->getBody();
    EXPECT_EQ(result, req_body);
}
