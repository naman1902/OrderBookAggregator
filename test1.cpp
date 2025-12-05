#include "https_get_client.hpp"
#include <iostream>

//checking http client to get from api
int main() {
    HttpsGetClient http;

    try {
        //std::string url = "https://api.exchange.coinbase.com/products/BTC-USD/book?level=2";
        std::string url = "https://api.gemini.com/v1/book/BTCUSD";
        std::string bookJson = http.Get(url);

        std::cout << "Response:\n" << bookJson << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "HTTP Error: " << ex.what() << std::endl;
    }

    return 0;
}
