#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>

using namespace siesta;

TEST(siesta, multiple_servers_destroy)
{
    std::shared_ptr<server::Server> server1;
    std::shared_ptr<server::Server> server2;
    EXPECT_NO_THROW(server1 = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server1->start());

    EXPECT_NO_THROW(server2 = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server2->start());

    server2 = nullptr;
    server1 = nullptr;
}
