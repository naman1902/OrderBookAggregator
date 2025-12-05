#include <string>
#include <future>
#include <curl/curl.h>
#include <chrono>
#include <vector>


struct endpoint {
    std::string url;
    std::string exchange;
    int interval_ms;   // polling interval
    std::chrono::steady_clock::time_point last_fire;

    endpoint(const std::string& u, const std::string& ex, int interval)
        : url(u), exchange(ex), interval_ms(interval),
          last_fire(std::chrono::steady_clock::now()) {}
};

struct HttpResult {
    std::string exchange;
    std::string response;
};

struct BookEntry {
    double bid_price = 0.0;
    double bid_size  = 0.0;
    double ask_price = 0.0;
    double ask_size  = 0.0;
};

struct BookLevel {
    double price = 0.0;
    double size  = 0.0;
};

struct OrderBook {
    std::vector<BookLevel> bids;
    std::vector<BookLevel> asks;
};

struct ExecStep {
   std::string exchange;
    double price;
    double quantity;
};
