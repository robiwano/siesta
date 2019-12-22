#include <gtest/gtest.h>
#include <siesta/server.h>

TEST(siesta, server_create)
{
    auto server = siesta::createServer("127.0.0.1", 8080);

    siesta::RouteHolder routeHolder;
    routeHolder += server->addRoute(
        siesta::Method::POST,
        "/my/test/path",
        [](const siesta::Request& req, siesta::Response& resp) {
            //
            resp.setBody(req.getBody());
        });
}
