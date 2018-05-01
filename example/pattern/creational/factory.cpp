#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>

using namespace std;

class Stock {
public:
    Stock(const string& name) : name_(name) { cout << "Stock(const string& name) " << name_ << endl; }

    Stock(Stock& s) = delete;

    ~Stock() { cout << "~Stock() " << name_ << endl; }

    const string& key() const { return name_; }

private:
    string name_;
};

/**
 * requirement:
 * Factory will get a shared_ptr of a stock object if exist
 * otherwise it will create a new one and then return
 * Factory only keep a week_ptr of that object,
 * so if shared_ptr which returned back to client finished life cycle,
 * it will call weakDeleteCallback to remove itself from Factory
 * no matter who dies first, Factory or Stock, no impact for reclaim the memory
 */
class StockFactory : public std::enable_shared_from_this<StockFactory> {
public:
    std::shared_ptr<Stock> get(const string& key) {
        std::shared_ptr<Stock> pStock;
        std::lock_guard<std::mutex> lock(mutex_);
        std::weak_ptr<Stock>& wkStock = stocks_[key];
        pStock = wkStock.lock();  // try to promote
        if (!pStock) {
            pStock.reset(new Stock(key),
                         std::bind(&StockFactory::weakDeleteCallback, std::weak_ptr<StockFactory>(shared_from_this()),
                                   std::placeholders::_1));
            wkStock = pStock;
        }
        return pStock;
    }

    StockFactory() = default;
    StockFactory(StockFactory& sf) = delete;

private:
    static void weakDeleteCallback(const std::weak_ptr<StockFactory>& wkFactory, Stock* stock) {
        cout << "weakDeleteStock\n";
        std::shared_ptr<StockFactory> factory(wkFactory.lock());
        if (factory) {
            factory->removeStock(stock);
        } else {
            cout << "factory died.\n";
        }
        delete stock;
    }

    void removeStock(Stock* stock) {
        if (stock) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = stocks_.find(stock->key());
            assert(it != stocks_.end());
            if (it->second.expired()) {
                stocks_.erase(stock->key());
            }
        }
    }

private:
    mutable std::mutex mutex_;
    std::map<string, std::weak_ptr<Stock> > stocks_;
};

void testLongLifeFactory() {
    std::shared_ptr<StockFactory> factory(new StockFactory);
    {
        std::shared_ptr<Stock> stock = factory->get("NYSE:IBM");
        std::shared_ptr<Stock> stock2 = factory->get("NYSE:IBM");
        assert(stock == stock2);
        // stock destructs here
    }
    // factory destructs here
}

void testShortLifeFactory() {
    std::shared_ptr<Stock> stock;
    {
        std::shared_ptr<StockFactory> factory(new StockFactory);
        stock = factory->get("NYSE:IBM");
        std::shared_ptr<Stock> stock2 = factory->get("NYSE:IBM");
        assert(stock == stock2);
        // factory destructs here
    }
    // stock destructs here
}

int main() {
    std::shared_ptr<StockFactory> sf5(new StockFactory);

    { std::shared_ptr<Stock> s5 = sf5->get("stock5"); }

    testLongLifeFactory();
    testShortLifeFactory();
    return 0;
}
