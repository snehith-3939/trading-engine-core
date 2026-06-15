#ifndef LIMIT_HPP
#define LIMIT_HPP

namespace exchange {


class TradeTicket;

class PriceLevel {
private:
    int levelPrice;
    int size;
    int totalVolume;
    int height;
    bool isBid;
    PriceLevel *parent;
    PriceLevel *leftChild;
    PriceLevel *rightChild;
    TradeTicket *headOrder;
    TradeTicket *tailOrder;

    friend class TradeTicket;
public:
    PriceLevel(int _limitPrice, bool _buyOrSell, int _size=0, int _totalVolume=0);
    ~PriceLevel();

    TradeTicket* getHeadOrder() const;
    int getLimitPrice() const;
    int getSize() const;
    int getTotalVolume() const;
    bool getBuyOrSell() const;
    PriceLevel* getParent() const;
    PriceLevel* getLeftChild() const;
    PriceLevel* getRightChild() const;
    int getHeight() const;
    void setHeight(int h);
    void setParent(PriceLevel* newParent);
    void setLeftChild(PriceLevel* newLeftChild);
    void setRightChild(PriceLevel* newRightChild);
    void partiallyFillTotalVolume(int orderedShares);

    void append(TradeTicket *_order);

    void printForward() const;
    void printBackward() const;
    void print() const;
};


} // namespace exchange

#endif