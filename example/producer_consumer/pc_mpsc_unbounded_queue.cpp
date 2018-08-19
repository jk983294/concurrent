#include <log/AsyncLog.h>
#include <lockfree/MpscUnboundedQueue.h>

using namespace std;

class Element : public frenzy::MpscUnboundedQueue<Element>::Node {
public:
    int value;
    Element(int val) : value(val){};
};

void produce(frenzy::MpscUnboundedQueue<Element>& q, int id) {
    Element* node;
    int value = 0;
    while (true) {
        ASYNC_LOG("producer " << id << " put " << value);
        node = new Element(value++);
        q.push(node);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 6)));
    }
}

void consume(frenzy::MpscUnboundedQueue<Element>& q, int id) {
    int data;
    while (true) {
        Element* e = q.pop();
        if (e) {
            data = e->value;
            ASYNC_LOG("consumer " << id << " get " << data);
            delete e;
            e = nullptr;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 3)));
    }
}

int main() {
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 1;

    frenzy::MpscUnboundedQueue<Element> q;

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
