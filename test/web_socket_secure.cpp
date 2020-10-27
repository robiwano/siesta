#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>

using namespace siesta;

namespace
{
    // This object will be created when a client connects to the websocket
    // and destroyed when disconnected.
    struct MySocketImpl : server::websocket::Reader {
        server::websocket::Writer& writer;
        MySocketImpl(server::websocket::Writer& w) : writer(w) {}
        void onReadData(const std::string& data) override
        {
            writer.writeData(data);
        }
    };
}  // namespace

TEST(siesta, websocket_secure_echo)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("https://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

    server::TokenHolder holder;
    EXPECT_NO_THROW(holder += server->addWebsocket(
                        "/socket", [](server::websocket::Writer& w) {
                            return new MySocketImpl(w);
                        }));

    const std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    std::unique_ptr<client::websocket::Writer> client;

    std::mutex m;
    std::condition_variable cv;
    std::string result;
    auto fn_read_callback = [&](const std::string& data) {
        std::lock_guard<std::mutex> lock(m);
        result = data;
        cv.notify_one();
    };

    EXPECT_NO_THROW(client = client::websocket::connect(
                        "wss://127.0.0.1:8080/socket", fn_read_callback));
    EXPECT_NO_THROW(client->writeData(req_body));

    std::unique_lock<std::mutex> lock(m);
    EXPECT_TRUE(cv.wait_for(
        lock, std::chrono::milliseconds(500), [&] { return !result.empty(); }));
    EXPECT_EQ(result, req_body);
}
