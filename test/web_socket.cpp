#include <gtest/gtest.h>

#include <siesta/client.h>
#include <siesta/server.h>

#include <nng/nng.h>

#include <thread>

using namespace siesta;

namespace
{
    constexpr auto num_nng_threads = 4;

    struct setup_nng {
        setup_nng()
        {
            nng_init_set_parameter(NNG_INIT_NUM_TASK_THREADS, num_nng_threads);
            nng_init_set_parameter(NNG_INIT_NUM_EXPIRE_THREADS,
                                   num_nng_threads);
            nng_init_set_parameter(NNG_INIT_NUM_POLLER_THREADS,
                                   num_nng_threads);
        }
    };

    static setup_nng _init_nng;

    // This object will be created when a client connects to the websocket
    // and destroyed when disconnected.
    struct MySocketImpl : server::websocket::Reader {
        server::websocket::Writer& writer;
        std::atomic<bool>* destructor_called_;
        MySocketImpl(server::websocket::Writer& w,
                     std::atomic<bool>* destructor_called = nullptr)
            : writer(w), destructor_called_(destructor_called)
        {
        }
        ~MySocketImpl()
        {
            if (destructor_called_) {
                *destructor_called_ = true;
            }
        }
        void onMessage(const std::string& data) override { writer.send(data); }
    };

    std::string get_address(const std::string& scheme, int port = 0)
    {
        return scheme + "://127.0.0.1:" + std::to_string(port);
    }
}  // namespace

TEST(websocket, echo)
{
    std::shared_ptr<server::Server> server;

    EXPECT_NO_THROW(server = server::createServer(get_address("http"), false));
    const int port = server->port();

    server::TokenHolder holder;
    EXPECT_NO_THROW(holder += server->addTextWebsocket(
                        "/socket", [](server::websocket::Writer& w) {
                            return new MySocketImpl(w);
                        }));

    const std::string req_body("{33F949DE-ED30-450C-B903-670EFF210D08}");
    std::unique_ptr<client::websocket::Writer> client;

    std::promise<std::string> result;
    auto f = result.get_future();

    auto fn_read_callback = [&](client::websocket::Writer&,
                                const std::string& data) {
        result.set_value(data);
    };

    ASSERT_NO_THROW(client = client::websocket::connect(
                        get_address("ws", port) + "/socket", fn_read_callback));
    EXPECT_NO_THROW(client->send(req_body));

    EXPECT_EQ(f.get(), req_body);
}

TEST(websocket, one_client_only)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http"), true));
    const int port = server->port();

    server::TokenHolder holder;
    EXPECT_NO_THROW(
        holder += server->addTextWebsocket(
            "/socket",
            [](server::websocket::Writer& w) { return new MySocketImpl(w); },
            1 /* Limit to one connection */));

    std::unique_ptr<client::websocket::Writer> client1;
    std::unique_ptr<client::websocket::Writer> client2;

    auto fn_read_callback = [&](client::websocket::Writer&,
                                const std::string& data) {};

    // First connection ok
    const auto client_addr = get_address("ws", port) + "/socket";
    EXPECT_NO_THROW(
        client1 = client::websocket::connect(client_addr, fn_read_callback));

    // Second connection shall fail
    EXPECT_THROW(
        client2 = client::websocket::connect(client_addr, fn_read_callback),
        std::runtime_error);

    // Release first connection
    client1 = nullptr;

    // Allow for server to shut down stream
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Try second connection again
    EXPECT_NO_THROW(
        client2 = client::websocket::connect(client_addr, fn_read_callback));

    holder.clear();
}

TEST(websocket, max_two_clients)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http"), true));
    const int port = server->port();

    server::TokenHolder holder;
    EXPECT_NO_THROW(
        holder += server->addTextWebsocket(
            "/socket",
            [](server::websocket::Writer& w) { return new MySocketImpl(w); },
            2 /* Limit to two connections */));

    std::unique_ptr<client::websocket::Writer> client1;
    std::unique_ptr<client::websocket::Writer> client2;
    std::unique_ptr<client::websocket::Writer> client3;

    auto fn_read_callback = [&](client::websocket::Writer&,
                                const std::string& data) {};

    // First connection ok
    const auto client_addr = get_address("ws", port) + "/socket";
    EXPECT_NO_THROW(
        client1 = client::websocket::connect(client_addr, fn_read_callback));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Second connection too
    EXPECT_NO_THROW(
        client2 = client::websocket::connect(client_addr, fn_read_callback));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Third connection though shall fail
    EXPECT_THROW(
        client3 = client::websocket::connect(client_addr, fn_read_callback),
        std::runtime_error);

    // Release first connection
    client1 = nullptr;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Try third connection again
    EXPECT_NO_THROW(
        client3 = client::websocket::connect(client_addr, fn_read_callback));
}

TEST(websocket, open_close_client)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http"), true));
    const int port = server->port();

    std::atomic<bool> server_socket_closed{false};
    server::TokenHolder holder;
    EXPECT_NO_THROW(holder += server->addTextWebsocket(
                        "/socket", [&](server::websocket::Writer& w) {
                            return new MySocketImpl(w, &server_socket_closed);
                        }));

    std::unique_ptr<client::websocket::Writer> client;

    std::atomic<bool> open_called{false};
    std::atomic<bool> close_called{false};

    auto fn_open_callback = [&](client::websocket::Writer&) {
        open_called = true;
    };
    auto fn_read_callback  = [&](client::websocket::Writer&,
                                const std::string& data) {};
    auto fn_error_callback = [&](client::websocket::Writer&,
                                 const std::string& error) {};
    auto fn_close_callback = [&](client::websocket::Writer&) {
        close_called = true;
    };

    const auto client_addr = get_address("ws", port) + "/socket";
    EXPECT_NO_THROW(client = client::websocket::connect(client_addr,
                                                        fn_read_callback,
                                                        fn_open_callback,
                                                        fn_error_callback,
                                                        fn_close_callback));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(open_called);

    // This will close the websocket from the client side
    client = nullptr;
    EXPECT_TRUE(close_called);

    // Allow for server to close the websocket stream
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(server_socket_closed);

    holder.clear();
}

TEST(websocket, open_close_server)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http"), true));
    const int port = server->port();

    server::TokenHolder holder;
    EXPECT_NO_THROW(holder += server->addTextWebsocket(
                        "/socket", [](server::websocket::Writer& w) {
                            return new MySocketImpl(w);
                        }));

    std::unique_ptr<client::websocket::Writer> client;

    std::atomic<bool> open_called{false};
    std::atomic<bool> close_called{false};

    auto fn_open_callback = [&](client::websocket::Writer&) {
        open_called = true;
    };
    auto fn_read_callback  = [&](client::websocket::Writer&,
                                const std::string& data) {};
    auto fn_error_callback = [&](client::websocket::Writer&,
                                 const std::string& error) {};
    auto fn_close_callback = [&](client::websocket::Writer&) {
        close_called = true;
    };

    const auto client_addr = get_address("ws", port) + "/socket";
    EXPECT_NO_THROW(client = client::websocket::connect(client_addr,
                                                        fn_read_callback,
                                                        fn_open_callback,
                                                        fn_error_callback,
                                                        fn_close_callback));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(open_called);

    // This will close the websocket from the server side
    holder.clear();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(close_called);
}

TEST(websocket, no_open_close)
{
    std::unique_ptr<client::websocket::Writer> client;

    bool open_called  = false;
    bool close_called = false;

    auto fn_open_callback = [&](client::websocket::Writer&) {
        open_called = true;
    };
    auto fn_read_callback  = [&](client::websocket::Writer&,
                                const std::string& data) {};
    auto fn_error_callback = [&](client::websocket::Writer&,
                                 const std::string& error) {};
    auto fn_close_callback = [&](client::websocket::Writer&) {
        close_called = true;
    };

    const auto client_addr = get_address("ws", 8080) + "/socket";

    EXPECT_THROW(client = client::websocket::connect(client_addr,
                                                     fn_read_callback,
                                                     fn_open_callback,
                                                     fn_error_callback,
                                                     fn_close_callback),
                 std::runtime_error);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(open_called);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(close_called);
}
