#include <gtest/gtest.h>

#include <siesta/client.h>
#include <siesta/server.h>

#include <nng/nng.h>

#include <chrono>
#include <thread>

using namespace siesta;
using namespace std::chrono_literals;

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
        std::shared_ptr<server::websocket::Writer> writer;
        std::function<void()> destructor_fn;
        MySocketImpl(std::shared_ptr<server::websocket::Writer> w,
                     std::function<void()> fn = nullptr)
            : writer(w), destructor_fn(fn)
        {
        }
        ~MySocketImpl()
        {
            if (destructor_fn) {
                destructor_fn();
            }
        }
        void onMessage(const std::string& data) override { writer->send(data); }
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

    std::promise<void> destructor_called;
    server::TokenHolder holder;
    EXPECT_NO_THROW(
        holder += server->addTextWebsocket(
            "/socket",
            [&destructor_called](std::shared_ptr<server::websocket::Writer> w) {
                return std::make_unique<MySocketImpl>(
                    w, [&destructor_called] { destructor_called.set_value(); });
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

    ASSERT_NE(f.wait_for(100ms), std::future_status::timeout);

    EXPECT_EQ(f.get(), req_body);
}

TEST(websocket, open_close_client)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http"), true));
    const int port = server->port();

    std::promise<void> server_socket_closed;
    server::TokenHolder holder;
    EXPECT_NO_THROW(holder += server->addTextWebsocket(
                        "/socket",
                        [&server_socket_closed](
                            std::shared_ptr<server::websocket::Writer> w) {
                            return std::make_unique<MySocketImpl>(
                                w, [&server_socket_closed] {
                                    server_socket_closed.set_value();
                                });
                        }));

    std::unique_ptr<client::websocket::Writer> client;

    std::promise<void> open_called;
    std::promise<void> close_called;

    auto fn_open_callback = [&](client::websocket::Writer&) {
        open_called.set_value();
    };
    auto fn_read_callback  = [&](client::websocket::Writer&,
                                const std::string& data) {};
    auto fn_error_callback = [&](client::websocket::Writer&,
                                 const std::string& error) {};
    auto fn_close_callback = [&] { close_called.set_value(); };

    const auto client_addr = get_address("ws", port) + "/socket";
    EXPECT_NO_THROW(client = client::websocket::connect(client_addr,
                                                        fn_read_callback,
                                                        fn_open_callback,
                                                        fn_error_callback,
                                                        fn_close_callback));
    EXPECT_NE(open_called.get_future().wait_for(100ms),
              std::future_status::timeout);

    // This will close the websocket from the client side
    client = nullptr;
    EXPECT_NE(close_called.get_future().wait_for(100ms),
              std::future_status::timeout);

    // Allow for server to close the websocket stream
    EXPECT_NE(server_socket_closed.get_future().wait_for(500ms),
              std::future_status::timeout);

    holder.clear();
}

TEST(websocket, open_close_server)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer(get_address("http"), true));
    const int port = server->port();

    server::TokenHolder holder;
    EXPECT_NO_THROW(
        holder += server->addTextWebsocket(
            "/socket", [](std::shared_ptr<server::websocket::Writer> w) {
                return std::make_unique<MySocketImpl>(w);
            }));

    std::unique_ptr<client::websocket::Writer> client;

    std::promise<void> open_called;
    std::promise<void> close_called;

    auto fn_open_callback = [&](client::websocket::Writer&) {
        open_called.set_value();
    };
    auto fn_read_callback  = [&](client::websocket::Writer&,
                                const std::string& data) {};
    auto fn_error_callback = [&](client::websocket::Writer&,
                                 const std::string& error) {};
    auto fn_close_callback = [&] { close_called.set_value(); };

    const auto client_addr = get_address("ws", port) + "/socket";
    EXPECT_NO_THROW(client = client::websocket::connect(client_addr,
                                                        fn_read_callback,
                                                        fn_open_callback,
                                                        fn_error_callback,
                                                        fn_close_callback));
    EXPECT_NE(open_called.get_future().wait_for(100ms),
              std::future_status::timeout);

    // This will close the websocket from the server side
    holder.clear();

    EXPECT_NE(close_called.get_future().wait_for(500ms),
              std::future_status::timeout);
}

TEST(websocket, no_open_close)
{
    std::unique_ptr<client::websocket::Writer> client;

    std::promise<void> open_called;
    std::promise<void> close_called;

    auto fn_open_callback = [&](client::websocket::Writer&) {
        open_called.set_value();
    };
    auto fn_read_callback  = [&](client::websocket::Writer&,
                                const std::string& data) {};
    auto fn_error_callback = [&](client::websocket::Writer&,
                                 const std::string& error) {};
    auto fn_close_callback = [&] { close_called.set_value(); };

    const auto client_addr = get_address("ws", 8080) + "/socket";

    EXPECT_THROW(client = client::websocket::connect(client_addr,
                                                     fn_read_callback,
                                                     fn_open_callback,
                                                     fn_error_callback,
                                                     fn_close_callback),
                 std::runtime_error);
    EXPECT_EQ(open_called.get_future().wait_for(100ms),
              std::future_status::timeout);
    EXPECT_EQ(close_called.get_future().wait_for(100ms),
              std::future_status::timeout);
}
