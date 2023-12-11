#include <lockfree/MpscUnboundedQueue.h>
#include <log/AsyncLog.h>
#include <thread/TimerWheel.h>
#include <zerg_time.h>
#include <functional>

using namespace std;

void produce(frenzy::TimerWheel<>& q, int id) {
    int value = 0;
    while (true) {
        auto tm = ztool::utime();
        int sleep_tm = rand() % 6 + 1;
        auto tm2 = tm + sleep_tm * 1000 * 1000;
        ASYNC_LOG("producer " << id << " put " << value << " sleep " << sleep_tm);
        q.register_timer(
            [id, value](int64_t tm_) {
                ASYNC_LOG("consumer get producer " << id << " value " << value << " at " << tm_);
            },
            tm2);
        ++value;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * sleep_tm));
    }
}

void consume(frenzy::TimerWheel<>& q, int id) {
    while (true) {
        q.advance(ztool::utime());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 1;

    frenzy::TimerWheel<> wheel;

    vector<thread> producers, consumers;
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        std::thread producer(std::bind(produce, std::ref(wheel), i + 1));
        producers.push_back(std::move(producer));
    }

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        std::thread consumer(std::bind(&consume, std::ref(wheel), i + 1));
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
