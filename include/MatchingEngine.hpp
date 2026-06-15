#ifndef BOOK_HPP
#define BOOK_HPP

#include "MemoryArena.hpp"
#include "TradeTicket.hpp"
#include "PriceLevel.hpp"
#include <unordered_map>
#include <vector>
#include <random>
#include <unordered_set>

namespace exchange {


class MatchingEngine {
private:
    MemoryArena<TradeTicket> orderPool_;
    MemoryArena<PriceLevel> limitPool_;

    PriceLevel *buyTree;
    PriceLevel *sellTree;
    PriceLevel *bestAsk;
    PriceLevel *topBid;

    PriceLevel *stopBuyTree;
    PriceLevel *stopSellTree;
    PriceLevel *highestStopSell;
    PriceLevel *lowestStopBuy;

    std::unordered_map<int, TradeTicket*> orderMap;
    std::unordered_map<int, PriceLevel*> limitBuyMap;
    std::unordered_map<int, PriceLevel*> limitSellMap;
    std::unordered_map<int, PriceLevel*> stopMap;

    void addLimit(int levelPrice, bool isBid);
    void addStop(int stopPrice, bool isBid);
    PriceLevel* insert(PriceLevel* root, PriceLevel* limit, PriceLevel* parent=nullptr);
    PriceLevel* insertStop(PriceLevel* root, PriceLevel* limit, PriceLevel* parent=nullptr);
    void updateBookEdgeInsert(PriceLevel* newLimit);
    void updateStopBookEdgeInsert(PriceLevel* newStop);
    void updateBookEdgeRemove(PriceLevel* limit);
    void updateStopBookEdgeRemove(PriceLevel* stopLevel);
    void changeBookRoots(PriceLevel* limit);
    void changeStopBookRoots(PriceLevel* stopLevel);
    void deleteLimit(PriceLevel* limit);
    void deleteStopLevel(PriceLevel* limit);
    void deleteFromOrderMap(int ticketId);
    void deleteFromLimitMaps(int LimitPrice, bool isBid);
    void deleteFromStopMap(int StopPrice);
    int limitOrderAsMarketOrder(int ticketId, bool isBid, int quantity, int levelPrice);
    int stopOrderAsMarketOrder(int ticketId, bool isBid, int quantity, int stopPrice);
    int existingOrderAsMarketOrder(TradeTicket* headOrder, bool isBid);
    int stopLimitOrderAsLimitOrder(int ticketId, bool isBid, int quantity, int levelPrice, int stopPrice);
    void executeStopOrders(bool isBid);
    void stopLimitOrderToLimitOrder(TradeTicket* headOrder, bool isBid);
    void marketOrderHelper(int ticketId, bool isBid, int quantity);

    // Functions to balance AVL tree
    void updateLimitHeight(PriceLevel* limit);
    int limitHeightDifference(PriceLevel* limit);
    PriceLevel* rr_rotate(PriceLevel* limit);
    PriceLevel* ll_rotate(PriceLevel* limit);
    PriceLevel* lr_rotate(PriceLevel* limit);
    PriceLevel* rl_rotate(PriceLevel* limit);
    PriceLevel* balance(PriceLevel* limit);
    PriceLevel* rr_rotateStop(PriceLevel* limit);
    PriceLevel* ll_rotateStop(PriceLevel* limit);
    PriceLevel* lr_rotateStop(PriceLevel* limit);
    PriceLevel* rl_rotateStop(PriceLevel* limit);
    PriceLevel* balanceStop(PriceLevel* limit);

public:
    MatchingEngine();
    ~MatchingEngine();

    // Counts used in order book perforamce visualisations
    int executedTicketsCount=0;
    int AVLTreeBalanceCount=0;

    // Getter and setter functions
    PriceLevel* getBuyTree() const;
    PriceLevel* getSellTree() const;
    PriceLevel* getBestAsk() const;
    PriceLevel* getTopBid() const;
    PriceLevel* getStopBuyTree() const;
    PriceLevel* getStopSellTree() const;
    PriceLevel* getHighestStopSell() const;
    PriceLevel* getLowestStopBuy() const;

    // Functions for different types of orders
    void submitMarketTicket(int ticketId, bool isBid, int quantity);
    void submitLimitTicket(int ticketId, bool isBid, int quantity, int levelPrice);
    void cancelLimitTicket(int ticketId);
    void modifyLimitTicket(int ticketId, int newShares, int newLimit);
    void submitStopTicket(int ticketId, bool isBid, int quantity, int stopPrice);
    void cancelStopTicket(int ticketId);
    void modifyStopTicket(int ticketId, int newShares, int newStopPrice);
    void submitStopLimitTicket(int ticketId, bool isBid, int quantity, int levelPrice, int stopPrice);
    void cancelStopLimitTicket(int ticketId);
    void modifyStopLimitTicket(int ticketId, int newShares, int newLimitPrice, int newStopPrice);

    // Functions that needed to be public for testing purposes
    int getLimitHeight(PriceLevel* limit) const;
    TradeTicket* searchOrderMap(int ticketId) const;
    PriceLevel* searchLimitMaps(int levelPrice, bool isBid) const;
    PriceLevel* searchStopMap(int stopPrice) const;

    // Functions for visualising the order book
    void printLimit(int levelPrice, bool isBid) const;
    void printOrder(int ticketId) const;
    void printBookEdges() const;
    void displayEngineState() const;
    std::vector<int> inOrderTreeTraversal(PriceLevel* root) const;
    std::vector<int> preOrderTreeTraversal(PriceLevel* root) const;
    std::vector<int> postOrderTreeTraversal(PriceLevel* root) const;

    // Functions and data structures needed for generating sample data
    TradeTicket* getRandomOrder(int key, std::mt19937 gen) const;
    std::unordered_set<TradeTicket*> limitOrders;
    std::unordered_set<TradeTicket*> stopOrders;
    std::unordered_set<TradeTicket*> stopLimitOrders;
};


} // namespace exchange

#endif