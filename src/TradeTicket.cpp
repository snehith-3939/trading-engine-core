#include "TradeTicket.hpp"
#include "PriceLevel.hpp"
#include <iostream>

namespace exchange {


TradeTicket::TradeTicket(int _idNumber, bool _buyOrSell, int _shares, int _limit)
    : idNumber(_idNumber), isBid(_buyOrSell), quantity(_shares), limit(_limit), 
    nextOrder(nullptr), prevOrder(nullptr), parentLevel(nullptr) {}

int TradeTicket::getShares() const
{
    return quantity;
}

int TradeTicket::getOrderId() const
{
    return idNumber;
}

bool TradeTicket::getBuyOrSell() const
{
    return isBid;
}

int TradeTicket::getLimit() const
{
    return limit;
}

PriceLevel* TradeTicket::getParentLimit() const
{
    return parentLevel;
}

void TradeTicket::partiallyFillOrder(int orderedShares)
{
    quantity -= orderedShares;
    parentLevel->partiallyFillTotalVolume(orderedShares);
}

// Remove order from its parent limit
void TradeTicket::cancel()
{
    if (prevOrder == nullptr)
    {
        parentLevel->headOrder = nextOrder;
    } else
    {
        prevOrder->nextOrder = nextOrder;
    }
    if (nextOrder == nullptr)
    {
        parentLevel->tailOrder = prevOrder;
    } else
    {
        nextOrder->prevOrder = prevOrder;
    }

    parentLevel->totalVolume -= quantity;
    parentLevel->size -= 1;
}

// Execute head order
void TradeTicket::execute()
{
    parentLevel->headOrder = nextOrder;
    if (nextOrder == nullptr)
    {
        parentLevel->tailOrder = nullptr;
    } else
    {
        nextOrder->prevOrder = nullptr;
    }
    nextOrder = nullptr;
    prevOrder = nullptr;

    parentLevel->totalVolume -= quantity;
    parentLevel->size -= 1;
}

void TradeTicket::modifyOrder(int newShares, int newLimit)
{
    quantity = newShares;
    limit = newLimit;
    nextOrder = nullptr;
    prevOrder = nullptr;
    parentLevel = nullptr;
}

void TradeTicket::setShares(int newShares)
{
    quantity = newShares;
}

void TradeTicket::print() const
{
    std::cout << "TradeTicket ID: " << idNumber 
    << ", TradeTicket Type: " << (isBid == 1 ? "buy" : "sell") 
    << ", TradeTicket Size: " << quantity
    << ", TradeTicket PriceLevel: " << limit 
    << std::endl;
}

} // namespace exchange
