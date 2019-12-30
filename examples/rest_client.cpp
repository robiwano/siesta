
#include <siesta/client.h>

#include <iostream>

using namespace siesta;

int main(int argc, char** argv)
{
    try {
        auto f        = client::getRequest("http://127.0.0.1:9080/", 1000);
        auto response = f.get();
        std::cout << response->getBody() << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}