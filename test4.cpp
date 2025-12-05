#include "bookmanager_new.hpp"
#include <iostream>
#include <csignal>
#include <chrono>

static bool RUNNING = true;

void signal_handler(int) {
    RUNNING = false;
}

int main() {
    std::signal(SIGINT, signal_handler);

    BookManager bm;

    bm.AddExchange(
        "https://api.exchange.coinbase.com/products/BTC-USD/book?level=2",
        "coinbase", 
        200
    );

    // this is gemini top level book
    bm.AddExchange(
        "https://api.gemini.com/v1/book/BTCUSD",
        "gemini",
        300
    );


    //std::cout << "Starting BookManager worker thread…" << std::endl;
    bm.Start();

    while (RUNNING) {
        std::cout << "================ ORDER BOOKS ================\n";

        bm.PrintBooksTop5();

        std::cout << "=============================================\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Stopping…" << std::endl;
    bm.Stop();
    return 0;
}
