#ifndef GENERATEORDERS_HPP
#define GENERATEORDERS_HPP

#include <random>

namespace exchange {


class MatchingEngine;

class OrderGenerator {
private:
    MatchingEngine* book;
    int ticketId = 11001;

    // Seed for random number generation
    std::random_device rd;
    std::mt19937 gen;

    void market();
    void addLimit();
    void cancelLimit();
    void modifyLimit();
    void addLimitMarket();
    void addStop();
    void cancelStop();
    void modifyStop();
    void addStopLimit();
    void cancelStopLimit();
    void modifyStopLimit();

public:
    OrderGenerator(MatchingEngine* book);
    void createInitialOrders(int numberOfOrders, int centreOfBook);
    void createOrders(int numberOfOrders);
};


} // namespace exchange

#endif