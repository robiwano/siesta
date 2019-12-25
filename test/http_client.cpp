#include <gtest/gtest.h>
#include <siesta/client.h>

#include <chrono>

TEST(siesta, client_connect_ok)
{
    auto f = siesta::client::getRequest("http://postman-echo.com/get?foo1=bar1",
                                        5000);

    std::unique_ptr<siesta::client::Response> resp;
    EXPECT_NO_THROW(resp = f.get());
    EXPECT_EQ(resp->getStatus(), siesta::HttpStatus::OK);
    EXPECT_GT(resp->getBody().size(), 0);
}

TEST(siesta, client_connect_timeout)
{
    auto f = siesta::client::getRequest("http://blarf.info", 5000);

    using clock  = std::chrono::high_resolution_clock;
    auto t_start = clock::now();
    std::string res;
    EXPECT_THROW(res = f.get()->getBody(), std::runtime_error);
    auto t_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                          clock::now() - t_start)
                          .count();
    EXPECT_LT(t_duration, 500);
}
