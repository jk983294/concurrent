#include <AsyncLog.h>
#include <nonblock/mvcc.h>

using namespace std;

void produce(frenzy::mvcc<int>& shared_value, int id) {
    auto updater = [](size_t version, int const& value) { return value + 1; };

    int data;
    size_t version;
    while (true) {
        auto snapshot = shared_value.update(updater);
        data = snapshot->value;
        version = snapshot->version;
        ASYNC_LOG("producer " << id << " put " << data << " version " << version);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 6)));
    }
}

void consume(frenzy::mvcc<int>& shared_value, int id) {
    int data;
    size_t version;
    while (true) {
        auto snapshot = shared_value.current();
        data = snapshot->value;
        version = snapshot->version;
        ASYNC_LOG("consumer " << id << " get " << data << " version " << version);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 6)));
    }
}

int main() {
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 2;

    frenzy::mvcc<int> shared_value{0};

    vector<thread> producers, consumers;
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        std::thread producer(std::bind(produce, std::ref(shared_value), i + 1));
        producers.push_back(std::move(producer));
    }

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        std::thread consumer(std::bind(&consume, std::ref(shared_value), i + 1));
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
