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
std::vector<ExecStep> buyplan;
std::vector<ExecStep> sellplan;

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
        double buycost  = bm.BestBuyCrossExchange(qty,buyplan);
        double sellcost = bm.BestSellCrossExchange(qty,sellplan);

        if (buycost < 0)
            std::cout << "Not enough liquidity to BUY " << qty << " BTC across exchanges.\n";
        else{
            std::cout << "Buy " << qty << " BTC for total " << buycost << "\n";
            for (auto& step : buyplan) {
                std::cout << "  Buy " << step.quantity << " BTC from " << step.exchange 
                  << " at price " << step.price << "\n";
            }   
        }

        if (sellcost < 0)
            std::cout << "Not enough liquidity to SELL " << qty << " BTC across exchanges.\n";
        else {
            std::cout << "Sell " << qty << " BTC for total " << sellcost << "\n";
            for (auto& step : sellplan) {
                std::cout << "  sell " << step.quantity << " BTC from " << step.exchange 
                  << " at price " << step.price << "\n";
            } 
        }
    }

    // Sleep briefly before next iteration
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

std::cout << "Stopping BookManager…" << std::endl;
bm.Stop();

return 0;
}