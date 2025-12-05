## Dependencies

Install:

### Ubuntu / Debian
sudo apt install g++ curl libcurl4-openssl-dev libjson-c-dev

## FEDORA /RHEL
sudo dnf install gcc-c++ libcurl-devel json-c-devel

## macos
brew install curl json-c

## compile using
g++ orderbookaggregator.cpp -o orderbookaggregator -std=c++20 -lcurl -ljson-c -lpthread
## to run
./orderbookaggregator

### objective:
Get best exchange to buy/sell certain quantity of BTCUSD

### core components
- Multi-exchange Level-2 order book fetching  
- Rate-limited HTTP request handler  
- Unified JSON parser (Coinbase / Gemini / Kraken)  
- Thread-safe order book storage  
- Cross-exchange best buy and best sell algorithms  
- Automatic price-sorted bid/ask book  
- Execution plan output (exchange, price, quantity

## tests
includes test file at each checkpoint of though process
