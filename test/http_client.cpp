#include <gtest/gtest.h>
#include <siesta/client.h>

#include <chrono>

TEST(siesta, client_connect_ok)
{
    auto f = siesta::client::getRequest("http://postman-echo.com/get?foo1=bar1",
                                        5000);

    std::string resp;
    EXPECT_NO_THROW(resp = f.get());
    EXPECT_GT(resp.size(), 0);
}

TEST(siesta, client_connect_timeout)
{
    auto f = siesta::client::getRequest("http://blarf.info", 1000);

    using clock  = std::chrono::high_resolution_clock;
    auto t_start = clock::now();
    std::string res;
    EXPECT_THROW(res = f.get(), std::runtime_error);
    auto t_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                          clock::now() - t_start)
                          .count();
    EXPECT_LT(t_duration, 500);
}
