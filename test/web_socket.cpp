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

TEST(siesta, websocket_echo)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
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
                        "ws://127.0.0.1:8080/socket", fn_read_callback));
    EXPECT_NO_THROW(client->writeData(req_body));

    std::unique_lock<std::mutex> lock(m);
    EXPECT_TRUE(cv.wait_for(
        lock, std::chrono::milliseconds(500), [&] { return !result.empty(); }));
    EXPECT_EQ(result, req_body);
}

TEST(siesta, websocket_one_client_only)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

    server::TokenHolder holder;
    EXPECT_NO_THROW(
        holder += server->addWebsocket(
            "/socket",
            [](server::websocket::Writer& w) { return new MySocketImpl(w); },
            1 /* Limit to one connection */));

    std::unique_ptr<client::websocket::Writer> client1;
    std::unique_ptr<client::websocket::Writer> client2;

    auto fn_read_callback = [&](const std::string& data) {};

    // First connection ok
    EXPECT_NO_THROW(client1 = client::websocket::connect(
                        "ws://127.0.0.1:8080/socket", fn_read_callback));

    // Second connection shall fail
    EXPECT_THROW(client2 = client::websocket::connect(
                     "ws://127.0.0.1:8080/socket", fn_read_callback),
                 std::runtime_error);

    // Release first connection
    client1 = nullptr;

    // Try second connection again
    EXPECT_NO_THROW(client2 = client::websocket::connect(
                        "ws://127.0.0.1:8080/socket", fn_read_callback));
}

TEST(siesta, websocket_max_two_clients)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

    server::TokenHolder holder;
    EXPECT_NO_THROW(
        holder += server->addWebsocket(
            "/socket",
            [](server::websocket::Writer& w) { return new MySocketImpl(w); },
            2 /* Limit to two connections */));

    std::unique_ptr<client::websocket::Writer> client1;
    std::unique_ptr<client::websocket::Writer> client2;
    std::unique_ptr<client::websocket::Writer> client3;

    auto fn_read_callback = [&](const std::string& data) {};

    // First connection ok
    EXPECT_NO_THROW(client1 = client::websocket::connect(
                        "ws://127.0.0.1:8080/socket", fn_read_callback));

    // Second connection too
    EXPECT_NO_THROW(client2 = client::websocket::connect(
                        "ws://127.0.0.1:8080/socket", fn_read_callback));

    // Third connection though shall fail
    EXPECT_THROW(client3 = client::websocket::connect(
                     "ws://127.0.0.1:8080/socket", fn_read_callback),
                 std::runtime_error);

    // Release first connection
    client1 = nullptr;

    // Try third connection again
    EXPECT_NO_THROW(client3 = client::websocket::connect(
                        "ws://127.0.0.1:8080/socket", fn_read_callback));
}
