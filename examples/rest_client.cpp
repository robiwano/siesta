
#include <siesta/client.h>
using namespace siesta;

#include <iostream>

int main(int argc, char** argv)
{
    try {
        std::string address = "http://127.0.0.1:9080/api/get/my_api"; // only shutdown example
        std::vector<std::pair<std::string, std::string>> headers;
        headers.push_back(std::make_pair("api_key", "123456"));

        /* Post request */
        auto f = client::postRequest("http://127.0.0.1:9080/api/create/my_api", "{\"myapi\":\"ok\"}", "");
        std::cout << "Api created!" << std::endl; // Used to create a new fake api

        /* Get Request */
        f = client::getRequest(address); // 1000 ms & no headers specified.
        std::cout << "Without headers = " << f.get() << std::endl; // Will not work since no api key in the headers.

        /* Get Request with headers */
        f = client::getRequest(address, headers);
        std::cout << "With headers = " << f.get() << std::endl;  // Will work since the api key is there.

        /* Get Request with custom timeout & headers */
        f = client::getRequest(address, headers, 8000);
        std::cout << "With headers & timeout = " << f.get() << std::endl;  // Will work since the api key is there.

    } catch (std::exception& e) {
        printf("Exception = %s\n", e.what());
        exit(0xDEAD);
    }   
    return 0;
}
