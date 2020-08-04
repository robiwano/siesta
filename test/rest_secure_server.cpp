#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>

using namespace siesta;

TEST(siesta, server_create_secure)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("https://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

    server::TokenHolder holder;
    EXPECT_NO_THROW(
        holder += server->addRoute(
            HttpMethod::POST,
            "/my/test/path",
            [](const server::rest::Request& req, server::rest::Response& resp) {
                //
                resp.setBody(req.getBody());
            }));

    std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    auto f = client::postRequest(
        "https://127.0.0.1:8080/my/test/path", req_body, "", 5000);

    std::string result;
    EXPECT_NO_THROW(result = f.get());
    EXPECT_EQ(result, req_body);
}
