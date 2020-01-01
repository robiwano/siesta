
#include <siesta/client.h>
#include <iostream>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        std::string address = "http://127.0.0.1:9080/";
        if (argc > 1) {
            address = argv[1];
        }
        auto f        = client::getRequest(address, 1000);
        auto response = f.get();
        std::cout << response << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}