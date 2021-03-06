#include <lockfree/MpmcBoundedQueue.h>
#include <log/AsyncLog.h>
#include <functional>

using namespace std;

void produce(frenzy::MpmcBoundedQueue<int>& q, int id) {
    int value = 0;
    while (true) {
        ASYNC_LOG("producer " << id << " put " << value);
        q.push(value);
        value++;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 5)));
    }
}

void consume(frenzy::MpmcBoundedQueue<int>& q, int id) {
    int data;
    while (true) {
        q.pop(data);
        ASYNC_LOG("consumer " << id << " get " << data);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 5)));
    }
}

int main() {
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 2;

    frenzy::MpmcBoundedQueue<int> q(4);

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
