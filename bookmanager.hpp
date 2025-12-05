#pragma once
#include "rate_limited_http_handler.hpp"
#include <json-c/json.h>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>



class BookManager {
private:
    RateLimitedHttpHandler handler;

    std::unordered_map<std::string, BookEntry> books;
    std::mutex book_mutex;

    std::thread bookthread;
    std::atomic<bool> running{false};

public:
    BookManager() = default;

    void AddExchange(const std::string& url,
                     const std::string& exchange,
                     int interval_ms)
    {
        handler.AddUrl(url, exchange, interval_ms);
    }

    // =============================
    //  Parse JSON with sizes
    // =============================
    bool ParseJson(const std::string& exchange,
                   const std::string& resp,
                   BookEntry& out)
    {
        json_object* root = json_tokener_parse(resp.c_str());
        if (!root) return false;

        bool ok = false;

        // -------------------------
        // COINBASE
        // {
        //   "bid":"92959.56",
        //   "ask":"92959.57",
        //   "size":"0.00000537"
        // }
        // -------------------------
        if (exchange == "coinbase") {
            json_object *bid = nullptr, *ask = nullptr, *size = nullptr;

            if (json_object_object_get_ex(root, "bid", &bid) &&
                json_object_object_get_ex(root, "ask", &ask))
            {
                out.bid_price = std::stod(json_object_get_string(bid));
                out.ask_price = std::stod(json_object_get_string(ask));

                // Coinbase only provides ask-size directly
                if (json_object_object_get_ex(root, "size", &size)) {
                    out.ask_size = std::stod(json_object_get_string(size));
                    out.bid_size = 1.0;  // best-effort assumption
                }
                else {
                    out.bid_size = out.ask_size = 1.0;
                }
                ok = true;
            }
        }

        // -------------------------
        // GEMINI
        // {
        //   "bid":"92946.33",
        //   "ask":"92963.76",
        //   ...
        // }
        // Gemini does not give sizes → assume deep book
        // -------------------------
        else if (exchange == "gemini") {
            json_object *bid = nullptr, *ask = nullptr;

            if (json_object_object_get_ex(root, "bid", &bid) &&
                json_object_object_get_ex(root, "ask", &ask))
            {
                out.bid_price = std::stod(json_object_get_string(bid));
                out.ask_price = std::stod(json_object_get_string(ask));

                out.bid_size = 1.0;   // unknown → assume sufficient
                out.ask_size = 1.0;
                ok = true;
            }
        }

        // -------------------------
        // KRAKEN
        // {
        //   "result": {
        //     "XXBTZUSD": {
        //       "a":[ price, size, ... ],
        //       "b":[ price, size, ... ]
        //     }
        //   }
        // }
        // -------------------------
        else if (exchange == "kraken") {
            json_object* result = nullptr;

            if (json_object_object_get_ex(root, "result", &result)) {
                json_object_object_foreach(result, key, value) {

                    json_object* ask = nullptr;
                    json_object* bid = nullptr;

                    if (json_object_object_get_ex(value, "a", &ask) &&
                        json_object_object_get_ex(value, "b", &bid))
                    {
                        json_object* a0 = json_object_array_get_idx(ask, 0);
                        json_object* a1 = json_object_array_get_idx(ask, 1);
                        json_object* b0 = json_object_array_get_idx(bid, 0);
                        json_object* b1 = json_object_array_get_idx(bid, 1);

                        if (a0 && a1 && b0 && b1) {
                            out.ask_price = std::stod(json_object_get_string(a0));
                            out.ask_size  = std::stod(json_object_get_string(a1));
                            out.bid_price = std::stod(json_object_get_string(b0));
                            out.bid_size  = std::stod(json_object_get_string(b1));
                            ok = true;
                        }
                        break;
                    }
                }
            }
        }

        json_object_put(root);
        return ok;
    }


    void Start() {
        running = true;

        bookthread = std::thread([this]() {
            while (running) {
                auto updates = handler.http_update();

                for (const auto& r : updates) {
                    BookEntry entry;

                    if (ParseJson(r.exchange, r.response, entry)) {
                        std::lock_guard<std::mutex> lock(book_mutex);
                        books[r.exchange] = entry;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }

    void Stop() {
        running = false;
        if (bookthread.joinable()) bookthread.join();
    }

    // =============================
    //  Safe access
    // =============================
    std::unordered_map<std::string, BookEntry> GetBooks() {
        std::lock_guard<std::mutex> lock(book_mutex);
        return books;
    }

    void PrintBooks() {
        std::lock_guard<std::mutex> lock(book_mutex);

        for (auto& [exchange, b] : books) {
            std::cout << exchange
                      << " => BID: " << b.bid_price
                      << " (size: " << b.bid_size << ")   "
                      << "ASK: " << b.ask_price
                      << " (size: " << b.ask_size << ")\n";
        }
    }
};
