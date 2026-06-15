#ifndef ORDER_HPP
#define ORDER_HPP

namespace exchange {


class PriceLevel;

class TradeTicket {
private:
    int idNumber;
    bool isBid;
    int quantity;
    int limit;
    TradeTicket *nextOrder;
    TradeTicket *prevOrder;
    PriceLevel *parentLevel;

    friend class PriceLevel;
public:
    TradeTicket(int _idNumber, bool _buyOrSell, int _shares, int _limit);

    int getShares() const;
    int getOrderId() const;
    bool getBuyOrSell() const;
    int getLimit() const;
    PriceLevel* getParentLimit() const;

    void partiallyFillOrder(int orderedShares);
    void cancel();
    void execute();
    void modifyOrder(int newShares, int newLimit);
    void setShares(int newShares);

    void print() const;
};


} // namespace exchange

#endif