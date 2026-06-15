#include "OrderGenerator.hpp"
#include "MatchingEngine.hpp"
using namespace exchange;
#include <iostream>
#include <chrono>

int main() {
    MatchingEngine* book = new MatchingEngine();
    OrderGenerator generateOrders(book);

    generateOrders.createInitialOrders(10000, 300);

    constexpr int numOrders = 5000000;
    auto start = std::chrono::high_resolution_clock::now();
    generateOrders.createOrders(numOrders);
    auto stop = std::chrono::high_resolution_clock::now();

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    double tps = numOrders * 1e9 / durationNs.count();
    double avgLatencyNs = durationNs.count() / static_cast<double>(numOrders);

    std::cout << "Orders: " << numOrders << std::endl;
    std::cout << "Time: " << durationMs.count() << " ms" << std::endl;
    std::cout << "TPS: " << static_cast<long>(tps) << " orders/sec" << std::endl;
    std::cout << "Avg latency: " << static_cast<long>(avgLatencyNs) << " ns/order" << std::endl;

    delete book;
    return 0;
}