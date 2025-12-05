#include "rate_limited_http_handler.hpp"
#include <iostream>
#include <thread>

int main() {
    RateLimitedHttpHandler handler;

    handler.AddUrl(
        "https://api.exchange.coinbase.com/products/BTC-USD/ticker",
        "coinbase", 
        2000 // 2 s
    );

    handler.AddUrl(
        "https://api.gemini.com/v1/pubticker/btcusd",
        "gemini",
        3000 // 3 s
    );



    std::cout << "Starting rate-limited HTTP polling...\n";

    while (true) {
        auto results = handler.http_update();

        for (const auto& r : results) {
            std::cout << "[" << r.exchange << "] response = "
                      << r.response << " \n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}

