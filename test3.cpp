#include <iostream>
#include <thread>
#include <chrono>
#include "bookmanager.hpp"


void ComputeBestExecution(const std::unordered_map<std::string, BookEntry>& books, double qty) // task objective
{
    double best_buy_price  = 1e18;
    double best_sell_price = -1e18;
    std::string best_buy_ex, best_sell_ex;

    for (const auto& [ex, b] : books) {

        if (b.ask_size >= qty && b.ask_price < best_buy_price) {
            best_buy_price = b.ask_price;
            best_buy_ex = ex;
        }

        if (b.bid_size >= qty && b.bid_price > best_sell_price) {
            best_sell_price = b.bid_price;
            best_sell_ex = ex;
        }
    }

    std::cout << "\n=== BEST EXECUTION FOR qty=" << qty << " BTC ===\n";

    if (!best_buy_ex.empty())
        std::cout << "BUY  from: " << best_buy_ex
                  << " @ price " << best_buy_price
                  << " → cost: " << best_buy_price * qty << "\n";
    else
        std::cout << "BUY  → Not enough size available\n";

    if (!best_sell_ex.empty())
        std::cout << "SELL to: " << best_sell_ex
                  << " @ price " << best_sell_price
                  << " → receive: " << best_sell_price * qty << "\n";
    else
        std::cout << "SELL → Not enough size available\n";

    std::cout << "=============================================\n\n";
}

int main()
{
    BookManager bm;

    bm.AddExchange(
        "https://api.exchange.coinbase.com/products/BTC-USD/ticker",
        "coinbase",
        800
    );

    bm.AddExchange(
        "https://api.gemini.com/v1/pubticker/btcusd",
        "gemini",
        900
    );


    bm.Start();// run thread

    // Run for 20 seconds
    for (int i = 0; i < 20; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "\n============== LATEST BOOKS ==============\n";
        bm.PrintBooks();

        // Copy books safely
        auto books_copy = bm.GetBooks();

        ComputeBestExecution(books_copy, 2.0); // i have hardcoded to test
    }

    bm.Stop();
    return 0;
}
