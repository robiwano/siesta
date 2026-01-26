#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>

using namespace siesta;

TEST(siesta, multiple_servers_destroy)
{
    std::shared_ptr<server::Server> server1;
    std::shared_ptr<server::Server> server2;
    EXPECT_NO_THROW(server1 = server::createServer("http://127.0.0.1:0"));
    const int port = server1->port();

    EXPECT_NO_THROW(server2 = server::createServer("http://127.0.0.1:" +
                                                   std::to_string(port)));

    server2 = nullptr;
    server1 = nullptr;
}
