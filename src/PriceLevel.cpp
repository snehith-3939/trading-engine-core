#include "PriceLevel.hpp"
#include "TradeTicket.hpp"
#include <iostream>

namespace exchange {


PriceLevel::PriceLevel(int _limitPrice, bool _buyOrSell, int _size, int _totalVolume)
    : levelPrice(_limitPrice), isBid(_buyOrSell), size(_size), totalVolume(_totalVolume),
    height(1), parent(nullptr), leftChild(nullptr), rightChild(nullptr),
    headOrder(nullptr), tailOrder(nullptr) {}

PriceLevel::~PriceLevel()
{
    // Tree reconnection is handled by deleteLimit/deleteStopLevel before destruction
    // This destructor only needs to clear pointers to avoid dangling references
    // The actual tree removal happens in MatchingEngine::deleteLimit/deleteStopLevel
}

TradeTicket* PriceLevel::getHeadOrder() const
{
    return headOrder;
}

int PriceLevel::getLimitPrice() const
{
    return levelPrice;
}

int PriceLevel::getSize() const
{
    return size;
}

int PriceLevel::getTotalVolume() const
{
    return totalVolume;
}

bool PriceLevel::getBuyOrSell() const
{
    return isBid;
}

PriceLevel* PriceLevel::getParent() const
{
    return parent;
}

PriceLevel* PriceLevel::getLeftChild() const
{
    return leftChild;
}

PriceLevel* PriceLevel::getRightChild() const
{
    return rightChild;
}

int PriceLevel::getHeight() const
{
    return height;
}

void PriceLevel::setHeight(int h)
{
    height = h;
}

void PriceLevel::setParent(PriceLevel* newParent)
{
    parent = newParent;
}

void PriceLevel::setLeftChild(PriceLevel* newLeftChild)
{
    leftChild = newLeftChild;
}

void PriceLevel::setRightChild(PriceLevel* newRightChild)
{
    rightChild = newRightChild;
}

void PriceLevel::partiallyFillTotalVolume(int orderedShares)
{
    totalVolume -= orderedShares;
}

// Add an order to the limit
void PriceLevel::append(TradeTicket *order)
{
        if (headOrder == nullptr) {
            headOrder = tailOrder = order;
        } else {
            tailOrder->nextOrder = order;
            order->prevOrder = tailOrder;
            order->nextOrder = nullptr;
            tailOrder = order;
        }
        size += 1;
        totalVolume += order->getShares();
        order->parentLevel = this;
}

void PriceLevel::printForward() const
{
    TradeTicket* current = headOrder;
    while (current != nullptr) {
        std::cout << current->getOrderId() << " ";
        current = current->nextOrder;
    }
    std::cout << std::endl;
}

void PriceLevel::printBackward() const
{
    TradeTicket* current = tailOrder;
    while (current != nullptr) {
        std::cout << current->getOrderId() << " ";
        current = current->prevOrder;
    }
    std::cout << std::endl;
}

void PriceLevel::print() const
{
    std::cout << "PriceLevel Price: " << levelPrice 
    << ", PriceLevel Volume: " << totalVolume 
    << ", PriceLevel Size: " << size 
    << std::endl;
}


} // namespace exchange
