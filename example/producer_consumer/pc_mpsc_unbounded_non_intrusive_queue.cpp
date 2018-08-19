#include <log/AsyncLog.h>
#include <lockfree/MpscUnboundedNonIntrusiveQueue.h>

using namespace std;

struct MyType {
    uint64_t data = 0;
};

void produce(frenzy::MpscUnboundedNonIntrusiveQueue<MyType>& q, int id) {
    MyType t;
    while (true) {
        ASYNC_LOG("producer " << id << " put " << t.data);
        q.put(t);
        ++t.data;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 6)));
    }
}

void consume(frenzy::MpscUnboundedNonIntrusiveQueue<MyType>& q, int id) {
    MyType t;
    while (true) {
        if (q.get(t)) {
            ASYNC_LOG("consumer " << id << " get " << t.data);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 3)));
    }
}

int main() {
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 1;

    frenzy::MpscUnboundedNonIntrusiveQueue<MyType> q;

    vector<thread> producers, consumers;
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        std::thread producer(std::bind(produce, std::ref(q), i + 1));
        producers.push_back(std::move(producer));
    }

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        std::thread consumer(std::bind(&consume, std::ref(q), i + 1));
        consumers.push_back(std::move(consumer));
    }

    for (auto& producer : producers) {
        producer.join();
    }

    for (auto& consumer : consumers) {
        consumer.join();
    }
    return 0;
}
