#include <log/AsyncLog.h>
#include <Monitor.h>
#include <future>
#include <vector>

using namespace std;

class Account {
public:
    Account(int m_) : money(m_) {}

    void expend(int m) { money -= m; }
    void save(int m) { money += m; }

public:
    int money;
};

class TransactionManager {
public:
    void transaction(Account& a, Account& b, int money) {
        a.expend(money);
        b.save(money);
    }
};

int main() {
    Account a{100}, b{42};
    TransactionManager manager;
    frenzy::Monitor<TransactionManager&> monitor{manager};
    vector<future<void>> v;

    for (int i = 0; i < 5; ++i) {
        v.push_back(async([&, i] {
            monitor([&, i](TransactionManager& m) {
                // now below two action constitute a transaction within this monitor
                m.transaction(a, b, i);
                ASYNC_LOG("after transaction " << a.money << " <--> " << b.money);
            });
        }));
    }
    for (auto& f : v) f.wait();
    ASYNC_LOG("done");

    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}
