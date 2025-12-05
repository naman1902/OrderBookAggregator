

#include <curl/curl.h>
#include <stdexcept>
#include <string>


class HttpsGetClient {
private:
    CURL* curl_handle_;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        std::string* s = static_cast<std::string*>(userp);
        s->append(static_cast<char*>(contents), total);
        return total;
    }

    void ThrowIfError(CURLcode code, const std::string& where) {
        if (code != CURLE_OK) {
            throw std::runtime_error(where + " failed: " + curl_easy_strerror(code));
        }
    }

public:
    HttpsGetClient() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_handle_ = curl_easy_init();

        if (!curl_handle_) {
            throw std::runtime_error("Failed to initialize CURL handle");
        }

        curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 2L);
    }

    ~HttpsGetClient() {
        if (curl_handle_) {
            curl_easy_cleanup(curl_handle_);
        }
        curl_global_cleanup();
    }

    std::string Get(const std::string& url) {
        if (!curl_handle_) {
            throw std::runtime_error("CURL handle is null");
        }

        std::string response;
        curl_slist* headers = nullptr;

      
        headers = curl_slist_append(headers, "User-Agent: MyOrderBookClient/1.0");

        headers = curl_slist_append(headers, "Accept: application/json");

        ThrowIfError(curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str()), "CURLOPT_URL");
        ThrowIfError(curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers), "CURLOPT_HTTPHEADER");
        ThrowIfError(curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback), "CURLOPT_WRITEFUNCTION");
        ThrowIfError(curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response), "CURLOPT_WRITEDATA");

        CURLcode res = curl_easy_perform(curl_handle_);
        curl_slist_free_all(headers);

        ThrowIfError(res, "curl_easy_perform");

        return response;
    }
};

