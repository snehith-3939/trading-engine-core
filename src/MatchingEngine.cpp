#include "MatchingEngine.hpp"
#include "TradeTicket.hpp"
#include "PriceLevel.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <iterator>

namespace exchange {


MatchingEngine::MatchingEngine() : buyTree(nullptr), sellTree(nullptr), bestAsk(nullptr), topBid(nullptr), 
            stopBuyTree(nullptr), stopSellTree(nullptr), highestStopSell(nullptr), lowestStopBuy(nullptr){}

// When deleting the book need to ensure all used memory is freed
MatchingEngine::~MatchingEngine()
{
    for (auto& [id, order] : orderMap) {
        orderPool_.free(order);
    }
    orderMap.clear();

    for (auto& [levelPrice, limit] : limitBuyMap) {
        limitPool_.free(limit);
    }
    limitBuyMap.clear();

    for (auto& [levelPrice, limit] : limitSellMap) {
        limitPool_.free(limit);
    }
    limitSellMap.clear();

    for (auto& [stopPrice, stopLevel] : stopMap) {
        limitPool_.free(stopLevel);
    }
    stopMap.clear();
}

PriceLevel* MatchingEngine::getBuyTree() const
{
    return buyTree;
}

PriceLevel* MatchingEngine::getSellTree() const
{
    return sellTree;
}

PriceLevel* MatchingEngine::getBestAsk() const
{
    return bestAsk;
}

PriceLevel* MatchingEngine::getTopBid() const
{
    return topBid;
}

PriceLevel* MatchingEngine::getStopBuyTree() const
{
    return stopBuyTree;
}

PriceLevel* MatchingEngine::getStopSellTree() const
{
    return stopSellTree;
}

PriceLevel* MatchingEngine::getHighestStopSell() const
{
    return highestStopSell;
}

PriceLevel* MatchingEngine::getLowestStopBuy() const
{
    return lowestStopBuy;
}

// Execute a market order
void MatchingEngine::submitMarketTicket(int ticketId, bool isBid, int quantity)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    marketOrderHelper(ticketId, isBid, quantity);
    executeStopOrders(isBid);
}

// Add a new limit order to the book
void MatchingEngine::submitLimitTicket(int ticketId, bool isBid, int quantity, int levelPrice)
{
    AVLTreeBalanceCount = 0;
    // Account for order being executed immediately
    quantity = limitOrderAsMarketOrder(ticketId, isBid, quantity, levelPrice);
    
    if (quantity != 0) [[likely]]
    {
        TradeTicket* newOrder = orderPool_.alloc(ticketId, isBid, quantity, levelPrice);
        orderMap.emplace(ticketId, newOrder);

        auto& limitMap = isBid ? limitBuyMap : limitSellMap;
        auto it = limitMap.find(levelPrice);
        if (it == limitMap.end()) {
            addLimit(levelPrice, newOrder->getBuyOrSell());
            it = limitMap.find(levelPrice);
        }
        it->second->append(newOrder);
        // limitOrders.insert(newOrder);
    } else {
        executeStopOrders(isBid);
    }
}

// Delete a limit order from the book
void MatchingEngine::cancelLimitTicket(int ticketId)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    TradeTicket* order = searchOrderMap(ticketId);

    if (order != nullptr)
    {
        order->cancel();
            if (order->getParentLimit()->getSize() == 0)
            {   
                deleteLimit(order->getParentLimit());
            }
        deleteFromOrderMap(ticketId);
        orderPool_.free(order);
    }
}

// Modify an existing limit order
void MatchingEngine::modifyLimitTicket(int ticketId, int newShares, int newLimit)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    TradeTicket* order = searchOrderMap(ticketId);
    if (order != nullptr)
    {
        order->cancel();
            if (order->getParentLimit()->getSize() == 0)
            {
                deleteLimit(order->getParentLimit());
            }
        
        order->modifyOrder(newShares, newLimit);
        auto& limitMap = order->getBuyOrSell() ? limitBuyMap : limitSellMap;
        auto it = limitMap.find(newLimit);
        if (it == limitMap.end()) {
            addLimit(newLimit, order->getBuyOrSell());
            it = limitMap.find(newLimit);
        }
        it->second->append(order);
    }
}

// Add a stop order
void MatchingEngine::submitStopTicket(int ticketId, bool isBid, int quantity, int stopPrice)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    // Account for stop order being executed immediately
    quantity = stopOrderAsMarketOrder(ticketId, isBid, quantity, stopPrice);
    
    if (quantity != 0)
    {
        TradeTicket* newOrder = orderPool_.alloc(ticketId, isBid, quantity, 0);
        orderMap.emplace(ticketId, newOrder);

        auto stopIt = stopMap.find(stopPrice);
        if (stopIt == stopMap.end()) {
            addStop(stopPrice, newOrder->getBuyOrSell());
            stopIt = stopMap.find(stopPrice);
        }
        stopIt->second->append(newOrder);
        // stopOrders.insert(newOrder);
    }
}

// Delete an stop order from the stop book
void MatchingEngine::cancelStopTicket(int ticketId)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    TradeTicket* order = searchOrderMap(ticketId);

    if (order != nullptr)
    {
        order->cancel();
            if (order->getParentLimit()->getSize() == 0)
            {   
                deleteStopLevel(order->getParentLimit());
            }
        deleteFromOrderMap(ticketId);
        orderPool_.free(order);
    }
}

// Modify an existing stop order
void MatchingEngine::modifyStopTicket(int ticketId, int newShares, int newStopPrice)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    TradeTicket* order = searchOrderMap(ticketId);
    if (order != nullptr)
    {
        order->cancel();
            if (order->getParentLimit()->getSize() == 0)
            {
                deleteStopLevel(order->getParentLimit());
            }
        
        order->modifyOrder(newShares, 0);

        auto stopIt = stopMap.find(newStopPrice);
        if (stopIt == stopMap.end()) {
            addStop(newStopPrice, order->getBuyOrSell());
            stopIt = stopMap.find(newStopPrice);
        }
        stopIt->second->append(order);
    }
}

// Add a stop limit order
void MatchingEngine::submitStopLimitTicket(int ticketId, bool isBid, int quantity, int levelPrice, int stopPrice)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    // Account for stop limit order being executed immediately
    quantity = stopLimitOrderAsLimitOrder(ticketId, isBid, quantity, levelPrice, stopPrice);
    
    if (quantity != 0)
    {
        TradeTicket* newOrder = orderPool_.alloc(ticketId, isBid, quantity, levelPrice);
        orderMap.emplace(ticketId, newOrder);

        auto stopIt = stopMap.find(stopPrice);
        if (stopIt == stopMap.end()) {
            addStop(stopPrice, newOrder->getBuyOrSell());
            stopIt = stopMap.find(stopPrice);
        }
        stopIt->second->append(newOrder);
    }
}

void MatchingEngine::cancelStopLimitTicket(int ticketId)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    TradeTicket* order = searchOrderMap(ticketId);

    if (order != nullptr)
    {
        order->cancel();
            if (order->getParentLimit()->getSize() == 0)
            {   
                deleteStopLevel(order->getParentLimit());
            }
        deleteFromOrderMap(ticketId);
        orderPool_.free(order);
    }
}

// Modify an existing stop limit order
void MatchingEngine::modifyStopLimitTicket(int ticketId, int newShares, int newLimitPrice, int newStopPrice)
{
    executedTicketsCount = 0;
    AVLTreeBalanceCount = 0;
    TradeTicket* order = searchOrderMap(ticketId);
    if (order != nullptr)
    {
        order->cancel();
            if (order->getParentLimit()->getSize() == 0)
            {
                deleteStopLevel(order->getParentLimit());
            }
        
        order->modifyOrder(newShares, newLimitPrice);

        auto stopIt = stopMap.find(newStopPrice);
        if (stopIt == stopMap.end()) {
            addStop(newStopPrice, order->getBuyOrSell());
            stopIt = stopMap.find(newStopPrice);
        }
        stopIt->second->append(order);
    }
}

// Get the height of a limit (O(1) from stored value)
int MatchingEngine::getLimitHeight(PriceLevel* limit) const {
    return limit ? limit->getHeight() : 0;
}

// Search the order map to find an order
TradeTicket* MatchingEngine::searchOrderMap(int ticketId) const
{
    auto it = orderMap.find(ticketId);
    if (it != orderMap.end())
    {
        return it->second;
    }
    return nullptr;
}

// Search the limit maps to find a limit
PriceLevel* MatchingEngine::searchLimitMaps(int levelPrice, bool isBid) const
{
    auto& limitMap = isBid ? limitBuyMap : limitSellMap;
    auto it = limitMap.find(levelPrice);
    if (it != limitMap.end())
    {
        return it->second;
    }
    return nullptr;
}

// Search the stop map to find a stop level
PriceLevel* MatchingEngine::searchStopMap(int stopPrice) const
{
    auto it = stopMap.find(stopPrice);
    if (it != stopMap.end())
    {
        return it->second;
    }
    return nullptr;
}

void MatchingEngine::printLimit(int levelPrice, bool isBid) const
{
    searchLimitMaps(levelPrice, isBid)->print();
}

void MatchingEngine::printOrder(int ticketId) const
{
    searchOrderMap(ticketId)->print();
}

void MatchingEngine::printBookEdges() const
{
    std::cout << "Buy edge: " << topBid->getLimitPrice() 
    << "Sell edge: " << bestAsk->getLimitPrice() << std::endl;
}

// Print out all the limit and stop levels and their liquidity
void MatchingEngine::displayEngineState() const
{
    std::vector<int> vec = inOrderTreeTraversal(getStopBuyTree());
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i] << "-" << searchStopMap(vec[i])->getTotalVolume();
        if (i != 0 && i != vec.size()-1 && vec[i] < vec[i-1]) {
            throw std::runtime_error("Error: vector is error");
        }
        if (i != vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    vec = inOrderTreeTraversal(getStopSellTree());
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i] << "-" << searchStopMap(vec[i])->getTotalVolume();
        if (i != 0 && i != vec.size()-1 && vec[i] < vec[i-1]) {
            throw std::runtime_error("Error: Vector is error");
        }
        if (i != vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    vec = inOrderTreeTraversal(getBuyTree());
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i] << "-" << searchLimitMaps(vec[i], true)->getTotalVolume();
        if (i != 0 && i != vec.size()-1 && vec[i] < vec[i-1]) {
            throw std::runtime_error("Error: vector is error");
        }
        if (i != vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    vec = inOrderTreeTraversal(getSellTree());
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i] << "-" << searchLimitMaps(vec[i], false)->getTotalVolume();
        if (i != 0 && i != vec.size()-1 && vec[i] < vec[i-1]) {
            throw std::runtime_error("Error: Vector is error");
        }
        if (i != vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

// In order traversal of the binary search tree
std::vector<int> MatchingEngine::inOrderTreeTraversal(PriceLevel* root) const
{
    std::vector<int> result;
    if (root == nullptr)
        return result;

    std::vector<int> leftSubtree = inOrderTreeTraversal(root->getLeftChild());
    result.insert(result.end(), leftSubtree.begin(), leftSubtree.end());
    
    result.push_back(root->getLimitPrice());

    std::vector<int> rightSubtree = inOrderTreeTraversal(root->getRightChild());
    result.insert(result.end(), rightSubtree.begin(), rightSubtree.end());

    return result;
}

// Pre order traversal of the binary search tree
std::vector<int> MatchingEngine::preOrderTreeTraversal(PriceLevel* root) const
{
    std::vector<int> result;
    if (root == nullptr)
        return result;

    result.push_back(root->getLimitPrice());

    std::vector<int> leftSubtree = preOrderTreeTraversal(root->getLeftChild());
    result.insert(result.end(), leftSubtree.begin(), leftSubtree.end());
    
    std::vector<int> rightSubtree = preOrderTreeTraversal(root->getRightChild());
    result.insert(result.end(), rightSubtree.begin(), rightSubtree.end());

    return result;
}

// Post order traversal of the binary search tree
std::vector<int> MatchingEngine::postOrderTreeTraversal(PriceLevel* root) const
{
    std::vector<int> result;
    if (root == nullptr)
        return result;

    std::vector<int> leftSubtree = postOrderTreeTraversal(root->getLeftChild());
    result.insert(result.end(), leftSubtree.begin(), leftSubtree.end());
    
    std::vector<int> rightSubtree = postOrderTreeTraversal(root->getRightChild());
    result.insert(result.end(), rightSubtree.begin(), rightSubtree.end());
    
    result.push_back(root->getLimitPrice());

    return result;
}

// Return a random active order
// 0:PriceLevel, 1:Stop, 2:StopLimit
TradeTicket* MatchingEngine::getRandomOrder(int key, std::mt19937 gen) const
{
    if (key == 0)
    {
        if (limitOrders.size() > 10000)
        {
            // Generate a random index within the range of the hash set size
            std::uniform_int_distribution<> mapDist(0, limitOrders.size() - 1);
            int randomIndex = mapDist(gen);

            // Access the element at the random index directly
            auto it = limitOrders.begin();
            std::advance(it, randomIndex);
            return *it;
        }
        return nullptr;
    } else if (key == 1)
    {
        if (stopOrders.size() > 500)
        {
            // Generate a random index within the range of the hash set size
            std::uniform_int_distribution<> mapDist(0, stopOrders.size() - 1);
            int randomIndex = mapDist(gen);

            // Access the element at the random index directly
            auto it = stopOrders.begin();
            std::advance(it, randomIndex);
            return *it;
        }
        return nullptr;
    } else if (key == 2)
    {
        if (stopLimitOrders.size() > 500)
        {
            // Generate a random index within the range of the hash set size
            std::uniform_int_distribution<> mapDist(0, stopLimitOrders.size() - 1);
            int randomIndex = mapDist(gen);

            // Access the element at the random index directly
            auto it = stopLimitOrders.begin();
            std::advance(it, randomIndex);
            return *it;
        }
        return nullptr;
    }
    return nullptr;
}

// Add a new limit to the book
void MatchingEngine::addLimit(int levelPrice, bool isBid)
{
    auto& limitMap = isBid ? limitBuyMap : limitSellMap;
    auto& tree = isBid ? buyTree : sellTree;
    auto& bookEdge = isBid ? topBid : bestAsk;

    PriceLevel* newLimit = limitPool_.alloc(levelPrice, isBid);
    limitMap.emplace(levelPrice, newLimit);

    if (tree == nullptr)
    {
        tree = newLimit;
        bookEdge = newLimit;
    } else
    {
        tree = insert(tree, newLimit);
        updateBookEdgeInsert(newLimit);
    }
}

// Add a new stop level to the book
void MatchingEngine::addStop(int stopPrice, bool isBid)
{
    auto& tree = isBid ? stopBuyTree : stopSellTree;
    auto& bookEdge = isBid ? lowestStopBuy : highestStopSell;

    PriceLevel* newStop = limitPool_.alloc(stopPrice, isBid);
    stopMap.emplace(stopPrice, newStop);

    if (tree == nullptr)
    {
        tree = newStop;
        bookEdge = newStop;
    } else
    {
        tree = insertStop(tree, newStop);
        updateStopBookEdgeInsert(newStop);
    }
}

// Insert a limit into its binary search tree
PriceLevel* MatchingEngine::insert(PriceLevel* root, PriceLevel* limit, PriceLevel* parent)
{
    if (root == nullptr)
    {
        limit->setParent(parent);
        return limit;
    }
    if (limit->getLimitPrice() < root->getLimitPrice())
    {
        root->setLeftChild(insert(root->getLeftChild(), limit, root));
        updateLimitHeight(root);
        root = balance(root);
    } else if (limit->getLimitPrice() > root->getLimitPrice())
    {
        root->setRightChild(insert(root->getRightChild(), limit, root));
        updateLimitHeight(root);
        root = balance(root);
    }

    return root;
}

// Insert a limit into its stop binary search tree
PriceLevel* MatchingEngine::insertStop(PriceLevel* root, PriceLevel* limit, PriceLevel* parent)
{
    if (root == nullptr)
    {
        limit->setParent(parent);
        return limit;
    }
    if (limit->getLimitPrice() < root->getLimitPrice())
    {
        root->setLeftChild(insertStop(root->getLeftChild(), limit, root));
        updateLimitHeight(root);
        root = balanceStop(root);
    } else if (limit->getLimitPrice() > root->getLimitPrice())
    {
        root->setRightChild(insertStop(root->getRightChild(), limit, root));
        updateLimitHeight(root);
        root = balanceStop(root);
    }
    return root;
}

// Update the edge of the book if new limit is on edge of the book
void MatchingEngine::updateBookEdgeInsert(PriceLevel* newLimit)
{
    if (newLimit->getBuyOrSell())
    {
        if (newLimit->getLimitPrice() > topBid->getLimitPrice())
        {
            topBid = newLimit;
        }
    } else
    {
        if (newLimit->getLimitPrice() < bestAsk->getLimitPrice())
        {
            bestAsk = newLimit;
        }
    }
}

// Update the edge of the stop book if new stop is on edge of the book
void MatchingEngine::updateStopBookEdgeInsert(PriceLevel* newStop)
{
    if (newStop->getBuyOrSell())
    {
        if (newStop->getLimitPrice() < lowestStopBuy->getLimitPrice())
        {
            lowestStopBuy = newStop;
        }
    } else
    {
        if (newStop->getLimitPrice() > highestStopSell->getLimitPrice())
        {
            highestStopSell = newStop;
        }
    }
}

// Update the edge of the book if current edge of the book is emptied
void MatchingEngine::updateBookEdgeRemove(PriceLevel* limit)
{
    auto& bookEdge = limit->getBuyOrSell() ? topBid : bestAsk;
    auto& tree = limit->getBuyOrSell() ? buyTree : sellTree;

    if (limit == bookEdge)
    {
        if (bookEdge != tree)
        {
            if (limit->getBuyOrSell() && bookEdge->getLeftChild() != nullptr)
            {
                bookEdge = bookEdge->getLeftChild();
            } else if (!limit->getBuyOrSell() && bookEdge->getRightChild() != nullptr)
            {
                bookEdge = bookEdge->getRightChild();
            } else {
            bookEdge = bookEdge->getParent();
            }
        } else {
            if (limit->getBuyOrSell() && bookEdge->getLeftChild() != nullptr)
            {
                bookEdge = bookEdge->getLeftChild();
            } else if (!limit->getBuyOrSell() && bookEdge->getRightChild() != nullptr)
            {
                bookEdge = bookEdge->getRightChild();
            } else {
            bookEdge = nullptr;
            }
        }
    }
}

// Update the edge of the stop book if current edge of the stop book is emptied
void MatchingEngine::updateStopBookEdgeRemove(PriceLevel* stopLevel)
{
    auto& bookEdge = stopLevel->getBuyOrSell() ? lowestStopBuy : highestStopSell;
    auto& tree = stopLevel->getBuyOrSell() ? stopBuyTree : stopSellTree;
    
    if (stopLevel == bookEdge)
    {
        if (bookEdge != tree)
        {
            if (stopLevel->getBuyOrSell() && bookEdge->getRightChild() != nullptr)
            {
                bookEdge = bookEdge->getRightChild();
            } else if (!stopLevel->getBuyOrSell() && bookEdge->getLeftChild() != nullptr)
            {
                bookEdge = bookEdge->getLeftChild();
            } else {
            bookEdge = bookEdge->getParent();
            }
        } else {
            if (stopLevel->getBuyOrSell() && bookEdge->getRightChild() != nullptr)
            {
                bookEdge = bookEdge->getRightChild();
            } else if (!stopLevel->getBuyOrSell() && bookEdge->getLeftChild() != nullptr)
            {
                bookEdge = bookEdge->getLeftChild();
            } else {
                bookEdge = nullptr;
            }
        }
    }
}

// Change the root limit in the AVL tree if the root limit is deleted
void MatchingEngine::changeBookRoots(PriceLevel* limit){
    auto& tree = limit->getBuyOrSell() ? buyTree : sellTree;
    if (limit == tree)
    {
        if (limit->getRightChild() != nullptr)
        {
            tree = tree->getRightChild();
            while (tree->getLeftChild() != nullptr)
            {
                tree = tree->getLeftChild();
            }
        } else
        {
            tree = limit->getLeftChild();
        }
    }
}

// Change the root stop level in the AVL tree if the root stop level is deleted
void MatchingEngine::changeStopBookRoots(PriceLevel* stopLevel){
    auto& tree = stopLevel->getBuyOrSell() ? stopBuyTree : stopSellTree;
    if (stopLevel == tree)
    {
        if (stopLevel->getRightChild() != nullptr)
        {
            tree = tree->getRightChild();
            while (tree->getLeftChild() != nullptr)
            {
                tree = tree->getLeftChild();
            }
        } else
        {
            tree = stopLevel->getLeftChild();
        }
    }
}

// Delete a limit after it has been emptied
void MatchingEngine::deleteLimit(PriceLevel* limit)
{
    updateBookEdgeRemove(limit);
    deleteFromLimitMaps(limit->getLimitPrice(), limit->getBuyOrSell());
    
    // Disconnect limit from tree before freeing
    PriceLevel* parent = limit->getParent();
    PriceLevel* leftChild = limit->getLeftChild();
    PriceLevel* rightChild = limit->getRightChild();
    
    if (parent != nullptr)
    {
        bool isLeftChild = (limit->getLimitPrice() < parent->getLimitPrice());
        if (leftChild == nullptr)
        {
            if (isLeftChild) parent->setLeftChild(rightChild);
            else parent->setRightChild(rightChild);
            if (rightChild != nullptr) rightChild->setParent(parent);
        }
        else if (rightChild == nullptr)
        {
            if (isLeftChild) parent->setLeftChild(leftChild);
            else parent->setRightChild(leftChild);
            leftChild->setParent(parent);
        }
        else
        {
            // Two children: find successor
            PriceLevel* successor = rightChild;
            while (successor->getLeftChild() != nullptr)
            {
                successor = successor->getLeftChild();
            }
            if (successor->getParent() != limit)
            {
                successor->getParent()->setLeftChild(successor->getRightChild());
                if (successor->getRightChild() != nullptr)
                {
                    successor->getRightChild()->setParent(successor->getParent());
                }
                successor->setRightChild(rightChild);
                rightChild->setParent(successor);
            }
            successor->setLeftChild(leftChild);
            leftChild->setParent(successor);
            successor->setParent(parent);
            if (isLeftChild) parent->setLeftChild(successor);
            else parent->setRightChild(successor);
        }
    }
    else
    {
        // Root case - handled by changeBookRoots
        changeBookRoots(limit);
    }

    int levelPrice = limit->getLimitPrice();
    limitPool_.free(limit);
    while (parent != nullptr)
    {
        updateLimitHeight(parent);
        parent = balance(parent);
        if (parent->getParent() != nullptr)
        {
            if (parent->getParent()->getLimitPrice() > levelPrice)
            {
                parent->getParent()->setLeftChild(parent);
            } else {
                parent->getParent()->setRightChild(parent);
            }
        }
        parent = parent->getParent();
    }
}

// Delete a stop level after it has been emptied
void MatchingEngine::deleteStopLevel(PriceLevel* stopLevel)
{
    updateStopBookEdgeRemove(stopLevel);
    deleteFromStopMap(stopLevel->getLimitPrice());
    
    // Disconnect stopLevel from tree before freeing
    PriceLevel* parent = stopLevel->getParent();
    PriceLevel* leftChild = stopLevel->getLeftChild();
    PriceLevel* rightChild = stopLevel->getRightChild();
    
    if (parent != nullptr)
    {
        bool isLeftChild = (stopLevel->getLimitPrice() < parent->getLimitPrice());
        if (leftChild == nullptr)
        {
            if (isLeftChild) parent->setLeftChild(rightChild);
            else parent->setRightChild(rightChild);
            if (rightChild != nullptr) rightChild->setParent(parent);
        }
        else if (rightChild == nullptr)
        {
            if (isLeftChild) parent->setLeftChild(leftChild);
            else parent->setRightChild(leftChild);
            leftChild->setParent(parent);
        }
        else
        {
            // Two children: find successor
            PriceLevel* successor = rightChild;
            while (successor->getLeftChild() != nullptr)
            {
                successor = successor->getLeftChild();
            }
            if (successor->getParent() != stopLevel)
            {
                successor->getParent()->setLeftChild(successor->getRightChild());
                if (successor->getRightChild() != nullptr)
                {
                    successor->getRightChild()->setParent(successor->getParent());
                }
                successor->setRightChild(rightChild);
                rightChild->setParent(successor);
            }
            successor->setLeftChild(leftChild);
            leftChild->setParent(successor);
            successor->setParent(parent);
            if (isLeftChild) parent->setLeftChild(successor);
            else parent->setRightChild(successor);
        }
    }
    else
    {
        // Root case - handled by changeStopBookRoots
        changeStopBookRoots(stopLevel);
    }

    int stopPrice = stopLevel->getLimitPrice();
    limitPool_.free(stopLevel);
    while (parent != nullptr)
    {
        updateLimitHeight(parent);
        parent = balanceStop(parent);
        if (parent->getParent() != nullptr)
        {
            if (parent->getParent()->getLimitPrice() > stopPrice)
            {
                parent->getParent()->setLeftChild(parent);
            } else {
                parent->getParent()->setRightChild(parent);
            }
        }
        parent = parent->getParent();
    }
}

// Delete an order from the order map
void MatchingEngine::deleteFromOrderMap(int ticketId)
{
    orderMap.erase(ticketId);
}

// Delete a limit from the limit maps
void MatchingEngine::deleteFromLimitMaps(int levelPrice, bool isBid)
{
    auto& limitMap = isBid ? limitBuyMap : limitSellMap;
    limitMap.erase(levelPrice);
}

// Delete a stop level from the stop map
void MatchingEngine::deleteFromStopMap(int stopPrice)
{
    stopMap.erase(stopPrice);
}

// When a limit order overlaps with the highest buy or lowest sell, immediately
// execute it as if it were a market order (inline marketOrderHelper to reduce call overhead)
int MatchingEngine::limitOrderAsMarketOrder(int ticketId, bool isBid, int quantity, int levelPrice)
{
    if (isBid) [[likely]]
    {
        PriceLevel*& bookEdge = bestAsk;
        while (bookEdge != nullptr && quantity != 0 && bookEdge->getLimitPrice() <= levelPrice)
        {
            PriceLevel* currentLimit = bookEdge;
            int limitVolume = currentLimit->getTotalVolume();
            
            if (quantity <= limitVolume)
            {
                // Execute remaining quantity using inline marketOrderHelper logic
                while (bookEdge != nullptr && bookEdge->getHeadOrder()->getShares() <= quantity)
                {
                    TradeTicket* headOrder = bookEdge->getHeadOrder();
                    quantity -= headOrder->getShares();
                    headOrder->execute();
                    if (bookEdge->getSize() == 0)
                    {
                        deleteLimit(bookEdge);
                    }
                    deleteFromOrderMap(headOrder->getOrderId());
                    orderPool_.free(headOrder);
                    executedTicketsCount += 1;
                }
                if (bookEdge != nullptr && quantity != 0)
                {
                    bookEdge->getHeadOrder()->partiallyFillOrder(quantity);
                    executedTicketsCount += 1;
                }
                return 0;
            } else {
                // Execute entire limit level - use marketOrderHelper to consume all orders
                quantity -= limitVolume;
                marketOrderHelper(ticketId, isBid, limitVolume);
            }
        }
        return quantity;
    } else [[unlikely]]
    {
        PriceLevel*& bookEdge = topBid;
        while (bookEdge != nullptr && quantity != 0 && bookEdge->getLimitPrice() >= levelPrice)
        {
            PriceLevel* currentLimit = bookEdge;
            int limitVolume = currentLimit->getTotalVolume();
            
            if (quantity <= limitVolume)
            {
                // Execute remaining quantity using inline marketOrderHelper logic
                while (bookEdge != nullptr && bookEdge->getHeadOrder()->getShares() <= quantity)
                {
                    TradeTicket* headOrder = bookEdge->getHeadOrder();
                    quantity -= headOrder->getShares();
                    headOrder->execute();
                    if (bookEdge->getSize() == 0)
                    {
                        deleteLimit(bookEdge);
                    }
                    deleteFromOrderMap(headOrder->getOrderId());
                    orderPool_.free(headOrder);
                    executedTicketsCount += 1;
                }
                if (bookEdge != nullptr && quantity != 0)
                {
                    bookEdge->getHeadOrder()->partiallyFillOrder(quantity);
                    executedTicketsCount += 1;
                }
                return 0;
            } else {
                // Execute entire limit level - use marketOrderHelper to consume all orders
                quantity -= limitVolume;
                marketOrderHelper(ticketId, isBid, limitVolume);
            }
        }
        return quantity;
    }
}

// When a stop order overlaps with the highest buy or lowest sell, immediately
// execute it as if it were a market order (direct call to avoid recursion)
int MatchingEngine::stopOrderAsMarketOrder(int ticketId, bool isBid, int quantity, int stopPrice)
{
    if (isBid && bestAsk != nullptr && stopPrice <= bestAsk->getLimitPrice()) [[likely]]
    {
        executedTicketsCount = 0;
        AVLTreeBalanceCount = 0;
        marketOrderHelper(ticketId, true, quantity);
        executeStopOrders(true);
        return 0;
    } else if (!isBid && topBid != nullptr && stopPrice >= topBid->getLimitPrice()) [[unlikely]]
    {
        executedTicketsCount = 0;
        AVLTreeBalanceCount = 0;
        marketOrderHelper(ticketId, false, quantity);
        executeStopOrders(false);
        return 0;
    }
    return quantity;
}

// When a limit order that used to be a stop limit order overlaps with the highest buy or lowest sell, 
// immediately execute it as if it were a market order
int MatchingEngine::existingOrderAsMarketOrder(TradeTicket* headOrder, bool isBid)
{
    int quantity = headOrder->getShares();
    int ticketId = headOrder->getOrderId();
    int levelPrice = headOrder->getLimit();
    
    if (isBid) [[likely]]
    {
        while (bestAsk != nullptr && bestAsk->getLimitPrice() <= levelPrice)
        {
            if (quantity <= bestAsk->getTotalVolume())
            {
                deleteFromOrderMap(ticketId);
                orderPool_.free(headOrder);
                marketOrderHelper(ticketId, isBid, quantity);
                return 0;
            } else {
                quantity -= bestAsk->getTotalVolume();
                marketOrderHelper(ticketId, isBid, bestAsk->getTotalVolume());
            }
        }
        return quantity;
    } else [[unlikely]]
    {
        while (topBid != nullptr && topBid->getLimitPrice() >= levelPrice)
        {
            if (quantity <= topBid->getTotalVolume())
            {
                deleteFromOrderMap(ticketId);
                orderPool_.free(headOrder);
                marketOrderHelper(ticketId, isBid, quantity);
                return 0;
            } else {
                quantity -= topBid->getTotalVolume();
                marketOrderHelper(ticketId, isBid, topBid->getTotalVolume());
            }
        }
        return quantity;
    }
}

// When a stop limit order overlaps with the highest buy or lowest sell, immediately
// execute it as if it were a limit order
int MatchingEngine::stopLimitOrderAsLimitOrder(int ticketId, bool isBid, int quantity, int levelPrice, int stopPrice)
{
    if (isBid && bestAsk != nullptr && stopPrice <= bestAsk->getLimitPrice())
    {
        submitLimitTicket(ticketId, true, quantity, levelPrice);
        return 0;
    } else if (!isBid && topBid != nullptr && stopPrice >= topBid->getLimitPrice())
    {
        submitLimitTicket(ticketId, false, quantity, levelPrice);
        return 0;
    }
    return quantity;
}

// Executes any stop orders which need to be executed
void MatchingEngine::executeStopOrders(bool isBid)
{
    if (isBid) [[likely]]
    {
        // Execute any buy stop market orders
        // If the book is empty and can't complete stop market order then it just doesn't execute and is forgotten.
        while (lowestStopBuy != nullptr && (bestAsk == nullptr || lowestStopBuy->getLimitPrice() <= bestAsk->getLimitPrice()))
        {
            TradeTicket* headOrder = lowestStopBuy->getHeadOrder();
            if (headOrder->getLimit() == 0)
            {
                int quantity = headOrder->getShares();
                headOrder->execute();
                if (lowestStopBuy->getSize() == 0)
                {
                    deleteStopLevel(lowestStopBuy);
                }
                deleteFromOrderMap(headOrder->getOrderId());
                orderPool_.free(headOrder);
                marketOrderHelper(0, true, quantity);
            } else {
                // stopLimitOrders.erase(headOrder);
                stopLimitOrderToLimitOrder(headOrder, isBid);
            }
        }
    } else [[unlikely]]
    {
        // Execute any sell stop market orders
        // If the book is empty and can't complete stop market order then it just doesn't execute and is forgotten.
        while (highestStopSell != nullptr && (topBid == nullptr || highestStopSell->getLimitPrice() >= topBid->getLimitPrice()))
        {
            TradeTicket* headOrder = highestStopSell->getHeadOrder();
            if (headOrder->getLimit() == 0)
            {
                int quantity = headOrder->getShares();
                headOrder->execute();
                if (highestStopSell->getSize() == 0)
                {
                    deleteStopLevel(highestStopSell);
                }
                deleteFromOrderMap(headOrder->getOrderId());
                orderPool_.free(headOrder);
                marketOrderHelper(0, false, quantity);
            } else {
                // stopLimitOrders.erase(headOrder);
                stopLimitOrderToLimitOrder(headOrder, isBid);
            }
        }
    }
}

// Turn stop limit order into limit order
void MatchingEngine::stopLimitOrderToLimitOrder(TradeTicket* headOrder, bool isBid)
{
    auto& bookEdge = isBid ? lowestStopBuy : highestStopSell;
    headOrder->execute();
    if (bookEdge->getSize() == 0)
    {
        deleteStopLevel(bookEdge);
    }

    // Account for order being executed immediately - majority of cases
    int quantity = existingOrderAsMarketOrder(headOrder, isBid);
    
    if (quantity != 0)
    {
        headOrder->setShares(quantity);
        auto& limitMap = isBid ? limitBuyMap : limitSellMap;
        int levelPrice = headOrder->getLimit();
        auto it = limitMap.find(levelPrice);
        if (it == limitMap.end()) {
            addLimit(levelPrice, isBid);
            it = limitMap.find(levelPrice);
        }
        it->second->append(headOrder);
        // limitOrders.insert(headOrder);
    }
}

// Function which actually executes the market order.
// If the book is empty and can't complete market order then market order just doesn't execute and is forgotten
void MatchingEngine::marketOrderHelper(int ticketId, bool isBid, int quantity)
{
    PriceLevel*& bookEdge = isBid ? bestAsk : topBid;

    while (bookEdge != nullptr && bookEdge->getHeadOrder()->getShares() <= quantity)
    {
        TradeTicket* headOrder = bookEdge->getHeadOrder();
        quantity -= headOrder->getShares();
        headOrder->execute();
        if (bookEdge->getSize() == 0)
        {
            deleteLimit(bookEdge);
        }
        deleteFromOrderMap(headOrder->getOrderId());
        orderPool_.free(headOrder);
        executedTicketsCount += 1;
    }
    if (bookEdge != nullptr && quantity != 0)
    {
        bookEdge->getHeadOrder()->partiallyFillOrder(quantity);
        executedTicketsCount += 1;
    }
}

// Update height from children (O(1))
void MatchingEngine::updateLimitHeight(PriceLevel* limit) {
    int lh = limit->getLeftChild() ? limit->getLeftChild()->getHeight() : 0;
    int rh = limit->getRightChild() ? limit->getRightChild()->getHeight() : 0;
    limit->setHeight(1 + std::max(lh, rh));
}

// Get height difference between a limits children (O(1))
int MatchingEngine::limitHeightDifference(PriceLevel* limit) {
    int lh = limit->getLeftChild() ? limit->getLeftChild()->getHeight() : 0;
    int rh = limit->getRightChild() ? limit->getRightChild()->getHeight() : 0;
    return lh - rh;
}

// RR rotation for AVL restructure
PriceLevel* MatchingEngine::rr_rotate(PriceLevel* parent) {
    PriceLevel* newParent = parent->getRightChild();
    parent->setRightChild(newParent->getLeftChild());
    if (newParent->getLeftChild() != nullptr)
    {
        newParent->getLeftChild()->setParent(parent);
    }
    newParent->setLeftChild(parent);
    if (parent->getParent() != nullptr)
    {
        newParent->setParent(parent->getParent());
    } else {
        newParent->setParent(nullptr);
        auto& tree = parent->getBuyOrSell() ? buyTree : sellTree;
        tree = newParent;
    }
    parent->setParent(newParent);
    updateLimitHeight(parent);
    updateLimitHeight(newParent);
    return newParent;
}

// LL rotation for AVL restructure
PriceLevel* MatchingEngine::ll_rotate(PriceLevel* parent) {
    PriceLevel* newParent = parent->getLeftChild();
    parent->setLeftChild(newParent->getRightChild());
    if (newParent->getRightChild() != nullptr)
    {
        newParent->getRightChild()->setParent(parent);
    }
    newParent->setRightChild(parent);
    if (parent->getParent() != nullptr)
    {
        newParent->setParent(parent->getParent());
    } else {
        newParent->setParent(nullptr);
        auto& tree = parent->getBuyOrSell() ? buyTree : sellTree;
        tree = newParent;
    }
    parent->setParent(newParent);
    updateLimitHeight(parent);
    updateLimitHeight(newParent);
    return newParent;
}

// LR rotation for AVL restructure
PriceLevel* MatchingEngine::lr_rotate(PriceLevel* parent) {
    PriceLevel* newParent = parent->getLeftChild();
    parent->setLeftChild(rr_rotate(newParent));
    return ll_rotate(parent);
}

// RL rotation for AVL restructure
PriceLevel* MatchingEngine::rl_rotate(PriceLevel* parent) {
    PriceLevel* newParent = parent->getRightChild();
    parent->setRightChild(ll_rotate(newParent));
    return rr_rotate(parent);
}

// Check if the AVL tree needs to be restructured
PriceLevel* MatchingEngine::balance(PriceLevel* limit) {
    int bal_factor = limitHeightDifference(limit);
    if (bal_factor > 1) {
        if (limitHeightDifference(limit->getLeftChild()) >= 0)
            limit = ll_rotate(limit);
        else
            limit = lr_rotate(limit);
        AVLTreeBalanceCount += 1;
    } else if (bal_factor < -1) {
        if (limitHeightDifference(limit->getRightChild()) > 0)
            limit = rl_rotate(limit);
        else
            limit = rr_rotate(limit);
        AVLTreeBalanceCount += 1;
    }
    return limit;
}

// RR rotation for AVL stop tree restructure
PriceLevel* MatchingEngine::rr_rotateStop(PriceLevel* parent) {
    PriceLevel* newParent = parent->getRightChild();
    parent->setRightChild(newParent->getLeftChild());
    if (newParent->getLeftChild() != nullptr)
    {
        newParent->getLeftChild()->setParent(parent);
    }
    newParent->setLeftChild(parent);
    if (parent->getParent() != nullptr)
    {
        newParent->setParent(parent->getParent());
    } else {
        newParent->setParent(nullptr);
        auto& tree = parent->getBuyOrSell() ? stopBuyTree : stopSellTree;
        tree = newParent;
    }
    parent->setParent(newParent);
    updateLimitHeight(parent);
    updateLimitHeight(newParent);
    return newParent;
}

// LL rotation for AVL stop tree restructure
PriceLevel* MatchingEngine::ll_rotateStop(PriceLevel* parent) {
    PriceLevel* newParent = parent->getLeftChild();
    parent->setLeftChild(newParent->getRightChild());
    if (newParent->getRightChild() != nullptr)
    {
        newParent->getRightChild()->setParent(parent);
    }
    newParent->setRightChild(parent);
    if (parent->getParent() != nullptr)
    {
        newParent->setParent(parent->getParent());
    } else {
        newParent->setParent(nullptr);
        auto& tree = parent->getBuyOrSell() ? stopBuyTree : stopSellTree;
        tree = newParent;
    }
    parent->setParent(newParent);
    updateLimitHeight(parent);
    updateLimitHeight(newParent);
    return newParent;
}

// LR rotation for AVL stop tree restructure
PriceLevel* MatchingEngine::lr_rotateStop(PriceLevel* parent) {
    PriceLevel* newParent = parent->getLeftChild();
    parent->setLeftChild(rr_rotateStop(newParent));
    return ll_rotateStop(parent);
}

// RL rotation for AVL stop tree restructure
PriceLevel* MatchingEngine::rl_rotateStop(PriceLevel* parent) {
    PriceLevel* newParent = parent->getRightChild();
    parent->setRightChild(ll_rotateStop(newParent));
    return rr_rotateStop(parent);
}

// Check if the AVL stop tree needs to be restructured
PriceLevel* MatchingEngine::balanceStop(PriceLevel* limit) {
    int bal_factor = limitHeightDifference(limit);
    if (bal_factor > 1) {
        if (limitHeightDifference(limit->getLeftChild()) >= 0)
            limit = ll_rotateStop(limit);
        else
            limit = lr_rotateStop(limit);
        AVLTreeBalanceCount += 1;
    } else if (bal_factor < -1) {
        if (limitHeightDifference(limit->getRightChild()) > 0)
            limit = rl_rotateStop(limit);
        else
            limit = rr_rotateStop(limit);
        AVLTreeBalanceCount += 1;
    }
    return limit;
}


} // namespace exchange
