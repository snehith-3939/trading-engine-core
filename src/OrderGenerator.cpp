#include "OrderGenerator.hpp"
#include "../include/MatchingEngine.hpp"
#include "../include/PriceLevel.hpp"
#include "../include/TradeTicket.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>
#include <functional>

namespace exchange {


OrderGenerator::OrderGenerator(MatchingEngine* _book) : book(_book), gen(rd()) {}

void OrderGenerator::market()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);

    int quantity = sharesDist(gen);
    bool isBid = buyOrSellDist(gen);

    book->submitMarketTicket(ticketId, isBid, quantity);
    ticketId ++;
}

void OrderGenerator::addLimit()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> limitPriceDist(300, 50);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);


    int quantity = sharesDist(gen);
    bool isBid = buyOrSellDist(gen);
    int levelPrice;

    if (isBid)
    {
        do {
            levelPrice = limitPriceDist(gen);
        } while (levelPrice >= book->getBestAsk()->getLimitPrice());  
    } else {
        do {
            levelPrice = limitPriceDist(gen);
        } while (levelPrice <= book->getTopBid()->getLimitPrice());  
    }

    book->submitLimitTicket(ticketId, isBid, quantity, levelPrice);
    ticketId ++;
}

void OrderGenerator::cancelLimit()
{
    TradeTicket* order = book->getRandomOrder(0, gen);

    if (order == nullptr)
    {
        addLimit();
        return;
    }
    int ticketId = order->getOrderId();
    book->cancelLimitTicket(ticketId);
}

void OrderGenerator::modifyLimit()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> limitPriceDist(book->getTopBid()->getLimitPrice(), 50);

    int quantity = sharesDist(gen);

    TradeTicket* order = book->getRandomOrder(0, gen);

    if (order == nullptr)
    {
        addLimit();
        return;
    }
    int ticketId = order->getOrderId();
    bool isBid = order->getBuyOrSell();
    int levelPrice;
    if (isBid)
    {
        do {
            levelPrice = limitPriceDist(gen);
        } while (levelPrice >= book->getBestAsk()->getLimitPrice());  
    } else {
        do {
            levelPrice = limitPriceDist(gen);
        } while (levelPrice <= book->getTopBid()->getLimitPrice());  
    }
    book->modifyLimitTicket(ticketId, quantity, levelPrice);
}

void OrderGenerator::addLimitMarket()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);
    std::normal_distribution<> limitPriceDist(book->getTopBid()->getLimitPrice(), 50);

    int quantity = sharesDist(gen);
    int levelPrice;
    bool isBid = buyOrSellDist(gen);

    if (isBid)
    {
        levelPrice = book->getBestAsk()->getLimitPrice() + 1;
    } else {
        levelPrice = book->getTopBid()->getLimitPrice() - 1;
    }
    
    book->submitLimitTicket(ticketId, isBid, quantity, levelPrice);
    ticketId ++;
}

void OrderGenerator::addStop()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> stopPriceDist(book->getTopBid()->getLimitPrice(), 50);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);


    int quantity = sharesDist(gen);
    bool isBid = buyOrSellDist(gen);

    int stopPrice;
    if (isBid)
    {
        do {
            stopPrice = stopPriceDist(gen);
        } while (stopPrice <= book->getBestAsk()->getLimitPrice());  
    } else {
        do {
            stopPrice = stopPriceDist(gen);
        } while (stopPrice >= book->getTopBid()->getLimitPrice());  
    }

    book->submitStopTicket(ticketId, isBid, quantity, stopPrice);
    ticketId ++;
}

void OrderGenerator::cancelStop()
{
    TradeTicket* order = book->getRandomOrder(1, gen);

    if (order == nullptr)
    {
        addStop();
        return;
    }
    int ticketId = order->getOrderId();
    book->cancelStopTicket(ticketId);
}

void OrderGenerator::modifyStop()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> stopPriceDist(book->getTopBid()->getLimitPrice(), 50);

    int quantity = sharesDist(gen);

    TradeTicket* order = book->getRandomOrder(1, gen);

    if (order == nullptr)
    {
        addStop();
        return;
    }
    int ticketId = order->getOrderId();
    bool isBid = order->getBuyOrSell();
    int stopPrice;
    if (isBid)
    {
        do {
            stopPrice = stopPriceDist(gen);
        } while (stopPrice <= book->getBestAsk()->getLimitPrice());  
    } else {
        do {
            stopPrice = stopPriceDist(gen);
        } while (stopPrice >= book->getTopBid()->getLimitPrice());  
    }
    book->modifyStopTicket(ticketId, quantity, stopPrice);
}

void OrderGenerator::addStopLimit()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> stopPriceDist(book->getTopBid()->getLimitPrice(), 50);
    std::uniform_int_distribution<> limitPriceDist(1, 5);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);


    int quantity = sharesDist(gen);
    bool isBid = buyOrSellDist(gen);

    int stopPrice;
    int levelPrice;
    if (isBid)
    {
        do {
            stopPrice = stopPriceDist(gen);
        } while (stopPrice <= book->getBestAsk()->getLimitPrice());  
        levelPrice = stopPrice + limitPriceDist(gen);
    } else {
        do {
            stopPrice = stopPriceDist(gen);
        } while (stopPrice >= book->getTopBid()->getLimitPrice());
        levelPrice = stopPrice - limitPriceDist(gen);
    }

    book->submitStopLimitTicket(ticketId, isBid, quantity, levelPrice, stopPrice);
    ticketId ++;
}

void OrderGenerator::cancelStopLimit()
{
    TradeTicket* order = book->getRandomOrder(2, gen);

    if (order == nullptr)
    {
        addStopLimit();
        return;
    }
    int ticketId = order->getOrderId();
    book->cancelStopLimitTicket(ticketId);
}

void OrderGenerator::modifyStopLimit()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> stopPriceDist(book->getTopBid()->getLimitPrice(), 50);
    std::uniform_int_distribution<> limitPriceDist(1, 5);

    int quantity = sharesDist(gen);

    TradeTicket* order = book->getRandomOrder(2, gen);

    if (order == nullptr)
    {
        addStopLimit();
        return;
    }
    int ticketId = order->getOrderId();
    bool isBid = order->getBuyOrSell();
    int stopPrice;
    int levelPrice;
    if (isBid)
    {
        do {
            stopPrice = stopPriceDist(gen);
        } while (stopPrice <= book->getBestAsk()->getLimitPrice());  
        levelPrice = stopPrice + limitPriceDist(gen);
    } else {
        do {
            stopPrice = stopPriceDist(gen);
        } while (stopPrice >= book->getTopBid()->getLimitPrice());  
        levelPrice = stopPrice - limitPriceDist(gen);
    }
    book->modifyStopLimitTicket(ticketId, quantity, levelPrice, stopPrice);
}

void OrderGenerator::createOrders(int numberOfOrders)
{
    std::uniform_real_distribution<> dis(0.0, 1.0);

    // Define the probabilities and actions
    std::vector<double> probabilities = {0.025, 0, 0.195, 0.295, 0.025, 0, 0.12, 0.12, 0, 0.12, 0.12};
    // std::vector<double> probabilities = {0.05, 0, 0.2, 0.3, 0, 0, 0.15, 0.15, 0, 0, 0.15};
    std::vector<std::function<void()>> actions = {
        std::bind(&OrderGenerator::market, this),
        std::bind(&OrderGenerator::addLimit, this),
        std::bind(&OrderGenerator::cancelLimit, this),
        std::bind(&OrderGenerator::modifyLimit, this),
        std::bind(&OrderGenerator::addLimitMarket, this),
        std::bind(&OrderGenerator::addStop, this),
        std::bind(&OrderGenerator::cancelStop, this),
        std::bind(&OrderGenerator::modifyStop, this),
        std::bind(&OrderGenerator::addStopLimit, this),
        std::bind(&OrderGenerator::cancelStopLimit, this),
        std::bind(&OrderGenerator::modifyStopLimit, this),
    };

    // Calculate the cumulative probabilities
    std::partial_sum(probabilities.begin(), probabilities.end(), probabilities.begin());

    for (size_t i = 1; i < numberOfOrders + 1; i++)
    {
        // Generate a random number between 0 and 1 
        double randNum = dis(gen);

        // Use std::lower_bound to find the first element greater than randNum
        auto it = std::lower_bound(probabilities.begin(), probabilities.end(), randNum);

        // Calculate the index of the selected action
        int selectedAction = std::distance(probabilities.begin(), it);

        // Perform the selected action
        if (selectedAction < probabilities.size()) {
            actions[selectedAction]();

            // if (i%100000 == 0)
            // {
            //     std::cout << "-------------------------------------" << std::endl;
            //     std::cout << "Number of orders done: " << i << std::endl;
                
            //     std::cout << "Highest Stop Sell: " << book->getHighestStopSell()->getLimitPrice() << ", Lowest Stop Buy: " << book->getLowestStopBuy()->getLimitPrice() << std::endl;
            //     std::cout << "Lowest Sell: " << book->getBestAsk()->getLimitPrice() << ", Highest Buy: " << book->getTopBid()->getLimitPrice() << std::endl;
            //     book->displayEngineState();
            // }
            
        } else {
            std::cerr << "Error: No action selected!" << std::endl;
        }
    }
}

void OrderGenerator::createInitialOrders(int numberOfOrders, int centreOfBook)
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> limitPriceDist(centreOfBook, 50);

    for (int order = 1; order <= numberOfOrders; ++order) {
        int quantity = sharesDist(gen);
        int levelPrice = limitPriceDist(gen);
        bool isBid = levelPrice < centreOfBook;
        book->submitLimitTicket(order, isBid, quantity, levelPrice);
    }

    std::uniform_int_distribution<> stopLimitPriceDist(1, 5);
    std::uniform_int_distribution<> stopOrStopLimitDist(0, 1);

    for (int order = numberOfOrders + 1; order <= numberOfOrders * 1.1; ++order) {
        int quantity = sharesDist(gen);
        int stopPrice = limitPriceDist(gen);
        bool isBid = stopPrice > centreOfBook;
        bool stopOrStopLimit = stopOrStopLimitDist(gen);

        if (stopOrStopLimit) {
            book->submitStopTicket(order, isBid, quantity, stopPrice);
        } else {
            int levelPrice = isBid ? stopPrice + stopLimitPriceDist(gen) : stopPrice - stopLimitPriceDist(gen);
            book->submitStopLimitTicket(order, isBid, quantity, levelPrice, stopPrice);
        }
    }
}

} // namespace exchange
