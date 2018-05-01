#include <utils/CountDownLatch.h>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

atomic<int> shared_variable{0};

void writer_thread(frenzy::CountDownLatch& latch_) {
    ++shared_variable;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 10)));
    latch_.countDown();
}

int main() {
    srand((unsigned)time(0));

    const int NUM_WRITERS = 10;
    frenzy::CountDownLatch latch_(NUM_WRITERS);

    vector<thread> writers;

    for (int i = 0; i < NUM_WRITERS; i++) {
        writers.push_back(thread(writer_thread, std::ref(latch_)));
    }

    latch_.wait();
    cout << "the result after all thread run is " << shared_variable.load() << endl;

    for (auto& writer : writers) {
        writer.join();
    }
    return 0;
}
