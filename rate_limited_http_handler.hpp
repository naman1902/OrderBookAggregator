#pragma once
#include "https_get_client.hpp"
#include "commondatastructures.hpp"
#include <string>
#include <chrono>
#include <vector>
#include <optional>
#include <iostream>



// to ask for api responses of books afer the interval for wait, rate limit has passed, http_update
class RateLimitedHttpHandler {
private:
    HttpsGetClient client_;
    std::vector<endpoint> endpoints;

public:
    RateLimitedHttpHandler() = default;

    void AddUrl(const std::string& url, const std::string& exchange, int interval_ms) {
        endpoints.emplace_back(url, exchange, interval_ms);
    }

    // refetching  responses - strings
    std::vector<HttpResult> http_update() {
        std::vector<HttpResult> results;
        auto now = std::chrono::steady_clock::now();

        for (auto& task : endpoints) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - task.last_fire).count();

            if (elapsed >= task.interval_ms) {
                try {
                    std::string response = client_.Get(task.url);
                    results.push_back({ task.exchange, response });
                    task.last_fire = now;
                } catch (const std::exception& e) {
                    // Log error but continue with next URLs
                    std::cerr << "Error fetching " << task.exchange << ": " << e.what() << "\n";
                }
            }
        }

        return results; ///not parsed
    }
};
