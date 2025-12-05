#include "bookmanager_new.hpp"
#include <iostream>
#include <csignal>
#include <chrono>
static bool RUNNING = true;

void signal_handler(int)  //  called when ctrl+c in terminal to exit
{  
RUNNING = false;
std::exit(0);
}

int main() {

std::signal(SIGINT, signal_handler);

BookManager bm;


bm.AddExchange("https://api.exchange.coinbase.com/products/BTC-USD/book?level=2", "coinbase", 200);
bm.AddExchange("https://api.gemini.com/v1/book/BTCUSD", "gemini", 300);


//std::cout << "Starting BookManager worker thread…" << std::endl;
bm.Start();

while (RUNNING) {
    

    std::cout << "Enter quantity to buy/sell (0 to skip, negative to exit): ";
    double qty;
    std::cin >> qty;

    if (!std::cin || qty < 0) {
        RUNNING = false;
        break;
    }

    std::cout << "\n================ ORDER BOOKS ================\n";
    bm.PrintBooksTop5();
    std::cout << "=============================================\n";

    if (qty == 0) {
        std::cout << "Skipping calculation for this round.\n";
    } else {
        double best_buy  = bm.BestBuy(qty);
        double best_sell = bm.BestSell(qty);

        if (best_buy < 0)
            std::cout << "Not enough liquidity to BUY " << qty << " BTC across exchanges.\n";
        else
            std::cout << "total cost to BUY " << qty << " BTC: $" << best_buy << "\n";

        if (best_sell < 0)
            std::cout << "Not enough liquidity to SELL " << qty << " BTC across exchanges.\n";
        else
            std::cout << "total cost to SELL " << qty << " BTC: $" << best_sell << "\n";
    }

    // Sleep briefly before next iteration
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

std::cout << "Stopping BookManager…" << std::endl;
bm.Stop();

return 0;
}