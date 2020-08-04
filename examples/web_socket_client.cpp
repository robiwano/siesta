
#include <siesta/client.h>
using namespace siesta;

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv)
{
    try {
        std::string addr = "ws://127.0.0.1:9080/test";
        if (argc > 1) {
            addr = argv[1];
        }
        auto fn_reader = [](const std::string& data) {
            std::cout << "Received: " << data << std::endl;
        };
        auto client = client::websocket::connect(addr, fn_reader);
        std::cout << "Client connected!" << std::endl;

        while (true) {
            std::string input;
            std::getline(std::cin, input);
            client->writeData(input);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
