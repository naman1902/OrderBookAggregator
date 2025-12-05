#pragma once
#include "rate_limited_http_handler.hpp"
#include <json-c/json.h>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>
#include <algorithm>



class BookManager {
private:
    RateLimitedHttpHandler rateapihandler;

    std::unordered_map<std::string, OrderBook> books;
    std::mutex book_mutex;

    std::thread bookthread;
    std::atomic<bool> running{false};
    const int MAX_LEVELS = 5;

public:
    BookManager() = default;

    void AddExchange(const std::string& url,
                     const std::string& exchange,
                     int interval_ms) {
        rateapihandler.AddUrl(url, exchange, interval_ms);
    }

  //response parser, the decoder stage
    bool ParseJson(const std::string& ex,
                   const std::string& resp,
                   OrderBook& ob)
    {
        json_object* root = json_tokener_parse(resp.c_str());
        if (!root) return false;

        bool ok = false;

        
        if (ex == "coinbase") {
            json_object *bids = nullptr, *asks = nullptr;

            if (json_object_object_get_ex(root, "bids", &bids) &&
                json_object_object_get_ex(root, "asks", &asks))
            {
                int nb = std::min(MAX_LEVELS, (int)json_object_array_length(bids));
                int na = std::min(MAX_LEVELS, (int)json_object_array_length(asks));

                for (int i = 0; i < nb; i++) {
                    json_object* lvl = json_object_array_get_idx(bids, i);
                    if (lvl && json_object_array_length(lvl) >= 2) {
                        double p = atof(json_object_get_string(json_object_array_get_idx(lvl, 0)));
                        double s = atof(json_object_get_string(json_object_array_get_idx(lvl, 1)));
                        ob.bids.push_back({p, s});
                    }
                }

                for (int i = 0; i < na; i++) {
                    json_object* lvl = json_object_array_get_idx(asks, i);
                    if (lvl && json_object_array_length(lvl) >= 2) {
                        double p = atof(json_object_get_string(json_object_array_get_idx(lvl, 0)));
                        double s = atof(json_object_get_string(json_object_array_get_idx(lvl, 1)));
                        ob.asks.push_back({p, s});
                    }
                }
                ok = true;
            }
        }

        // gemini parser for multiple levels, signle level parser didn't work
        if (ex == "gemini") { 
            json_object *bids = nullptr, *asks = nullptr; 
            if (json_object_object_get_ex(root, "bids", &bids) && json_object_object_get_ex(root, "asks", &asks)) { // Parse all bid levels 
                for (size_t i = 0; i < json_object_array_length(bids); i++) { 
                    json_object* lvl = json_object_array_get_idx(bids, i); 
                    json_object *p = nullptr, *s = nullptr; 
                    if (json_object_object_get_ex(lvl, "price", &p) && json_object_object_get_ex(lvl, "amount", &s)) { 
                        ob.bids.push_back({ atof(json_object_get_string(p)), atof(json_object_get_string(s)) }); 
                    }
                } // Parse all ask levels 
                for (size_t i = 0; i < json_object_array_length(asks); i++) { 
                    json_object* lvl = json_object_array_get_idx(asks, i); json_object *p = nullptr, *s = nullptr; 
                    if (json_object_object_get_ex(lvl, "price", &p) && json_object_object_get_ex(lvl, "amount", &s)) { 
                        ob.asks.push_back({ atof(json_object_get_string(p)), atof(json_object_get_string(s)) }); 
                    }
                } 
                ok = true; 
            }
        }

        std::sort(ob.bids.begin(), ob.bids.end(), [](auto& a, auto& b){return a.price > b.price;});
        std::sort(ob.asks.begin(), ob.asks.end(), [](auto& a, auto& b){return a.price < b.price;});

        json_object_put(root);
        return ok;
    }

    // bacground thread 
    void Start() {
        running = true;

        bookthread = std::thread([this]() {
            while (running) {
                auto updates = rateapihandler.http_update();

                for (auto& r : updates) {
                    OrderBook ob;
                    if (ParseJson(r.exchange, r.response, ob)) {
                        std::lock_guard<std::mutex> lock(book_mutex);
                        books[r.exchange] = ob;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }

    void Stop() {
        running = false;
        if (bookthread.joinable()) bookthread.join();
    }

    // this take all levels of bids from all exchanges and pushes into vector->sort , we have our combined book based on price.
    //contains multiple same price entries
    double BestBuy(double qty) {
        struct Node { double price; double size; };
        std::vector<Node> pool;

        {
            std::lock_guard<std::mutex> lock(book_mutex);
            for (auto& [ex, ob] : books) {
                for (auto& a : ob.asks)
                    pool.push_back({a.price, a.size});
            }
        }

        std::sort(pool.begin(), pool.end(),
                  [](auto& a, auto& b){return a.price < b.price;});

        double need = qty, cost = 0.0;
        for (auto& lvl : pool) {
            double take = std::min(need, lvl.size);
            cost += take * lvl.price;
            need -= take;
            if (need <= 0) break;
        }

        return (need > 0 ? -1.0 : cost);
    }

    double BestSell(double qty) {
        struct Node { double price; double size; };
        std::vector<Node> pool;

        {
            std::lock_guard<std::mutex> lock(book_mutex);
            for (auto& [ex, ob] : books) {
                for (auto& b : ob.bids)
                    pool.push_back({b.price, b.size});
            }
        }

        std::sort(pool.begin(), pool.end(),
                  [](auto& a, auto& b){return a.price > b.price;});

        double need = qty, revenue = 0.0;
        for (auto& lvl : pool) {
            double take = std::min(need, lvl.size);
            revenue += take * lvl.price;
            need -= take;
            if (need <= 0) break;
        }

        return (need > 0 ? -1.0 : revenue);
    }
    // this approach is to optimize above brute force approach, no need to go through all levels as book of 1 exchange is sorted already.
    double BestBuyCrossExchange(double qty, std::vector<ExecStep>& plan) {
    struct AskPointer { std::string ex; size_t idx; };

    std::lock_guard<std::mutex> lock(book_mutex);
    plan.clear();
    double remaining = qty;
    double total_cost = 0.0;

    std::vector<AskPointer> pointers;
    for (auto& [ex, ob] : books) {
        if (!ob.asks.empty())
            pointers.push_back({ex, 0});
    }

    while (remaining > 0 && !pointers.empty()) {
        size_t best_idx = 0;
        double best_price = books[pointers[0].ex].asks[pointers[0].idx].price;

        for (size_t i = 1; i < pointers.size(); ++i) {
            double price = books[pointers[i].ex].asks[pointers[i].idx].price;
            if (price < best_price) {
                best_price = price;
                best_idx = i;
            }
        }

        auto& ptr = pointers[best_idx];
        auto& lvl = books[ptr.ex].asks[ptr.idx];
        double take = std::min(remaining, lvl.size);

        plan.push_back({ptr.ex, lvl.price, take});
        total_cost += take * lvl.price;
        remaining -= take;

        ptr.idx++;
        if (ptr.idx >= books[ptr.ex].asks.size()) {
            pointers.erase(pointers.begin() + best_idx);
        }
    }

    return (remaining > 0 ? -1.0 : total_cost);
}


    double BestSellCrossExchange(double qty, std::vector<ExecStep>& plan) {
    struct BidPointer { std::string ex; size_t idx; };

    std::lock_guard<std::mutex> lock(book_mutex);
    plan.clear();
    double remaining = qty;
    double total_revenue = 0.0;

    std::vector<BidPointer> pointers;
    for (auto& [ex, ob] : books) {
        if (!ob.bids.empty())
            pointers.push_back({ex, 0});
    }

    while (remaining > 0 && !pointers.empty()) {
        size_t best_idx = 0;
        double best_price = books[pointers[0].ex].bids[pointers[0].idx].price;

        for (size_t i = 1; i < pointers.size(); ++i) {
            double price = books[pointers[i].ex].bids[pointers[i].idx].price;
            if (price > best_price) {
                best_price = price;
                best_idx = i;
            }
        }

        auto& ptr = pointers[best_idx];
        auto& lvl = books[ptr.ex].bids[ptr.idx];
        double take = std::min(remaining, lvl.size);

        plan.push_back({ptr.ex, lvl.price, take});
        total_revenue += take * lvl.price;
        remaining -= take;

        ptr.idx++;
        if (ptr.idx >= books[ptr.ex].bids.size()) {
            pointers.erase(pointers.begin() + best_idx);
        }
    }

    return (remaining > 0 ? -1.0 : total_revenue);
}

    void PrintBooks() {
        std::lock_guard<std::mutex> lock(book_mutex);

        for (auto& [ex, ob] : books) {
            std::cout << "\n=== " << ex << " ===\n";
            std::cout << "BIDS:\n";
            for (auto& b : ob.bids)
                std::cout << "  " << b.price << " x " << b.size << "\n";

            std::cout << "ASKS:\n";
            for (auto& a : ob.asks)
                std::cout << "  " << a.price << " x " << a.size << "\n";
        }
    }

    void PrintBooksTop5() {
    std::lock_guard<std::mutex> lock(book_mutex);

    for (auto& [ex, book] : books) {
        std::cout << ex << ":\n";
        std::cout << "BIDS\t\tSIZE\t\tASKS\t\tSIZE\n";

        size_t max_levels = std::min<size_t>(5, std::max(book.bids.size(), book.asks.size()));
        for (size_t i = 0; i < max_levels; ++i) {
            std::string bid_price = i < book.bids.size() ? std::to_string(book.bids[i].price) : "-";
            std::string bid_size  = i < book.bids.size() ? std::to_string(book.bids[i].size)  : "-";
            std::string ask_price = i < book.asks.size() ? std::to_string(book.asks[i].price) : "-";
            std::string ask_size  = i < book.asks.size() ? std::to_string(book.asks[i].size)  : "-";

            std::cout << bid_price << "\t" << bid_size << "\t" << ask_price << "\t" << ask_size << "\n";
        }
        std::cout << "--------------------------------------\n";
    }
}

};
