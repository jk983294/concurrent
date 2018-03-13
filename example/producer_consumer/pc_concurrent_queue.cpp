#include <AsyncLog.h>
#include <ConcurrentQueue.h>
#include <iostream>
#include <thread>

using namespace std;

void produce(frenzy::ConcurrentQueue<int>& q, int id) {
    int value = 0;
    while (true) {
        ASYNC_LOG("producer " << id << " put " << value);
        q.push(value);
        value++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 2)));
    }
}

void consume(frenzy::ConcurrentQueue<int>& q, int id) {
    while (true) {
        int item = q.pop();
        ASYNC_LOG("consumer " << id << " get " << item);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 10)));
    }
}

int main() {
    const int NUM_PRODUCERS = 1;
    const int NUM_CONSUMERS = 4;

    frenzy::ConcurrentQueue<int> q;

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
