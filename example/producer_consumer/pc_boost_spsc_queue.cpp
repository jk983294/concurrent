#include <AsioLog.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>
#include <thread>

using namespace std;

typedef boost::lockfree::spsc_queue<int, boost::lockfree::capacity<1024>> TQueue;

void produce(TQueue& q, int id) {
    int value = 0;
    while (true) {
        while (!q.push(value)) asm volatile("pause");
        ASIO_LOG_INFO("producer " << id << " put " << value);
        ++value;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 5)));
    }
}

void consume(TQueue& q, int id) {
    int data;
    while (true) {
        while (!q.pop(data)) asm volatile("pause");
        ASIO_LOG_INFO("consumer " << id << " get " << data);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 5)));
    }
}

int main() {
    const int NUM_PRODUCERS = 1;
    const int NUM_CONSUMERS = 1;

    TQueue q;

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
