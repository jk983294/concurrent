#include <AsyncLog.h>
#include <atomic>

using namespace std;

std::atomic<int> value;

void worker(int id) {
    while (1) {
        int localValue = value.load();
        ASYNC_LOG("thread " << id << " get value " << localValue);
        ++value;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (rand() % 5)));
    }
}

int main() {
    const int NUM_WORKER = 5;
    vector<thread> workers;

    for (int i = 0; i < NUM_WORKER; i++) {
        std::thread w(std::bind(&worker, i + 1));
        workers.push_back(std::move(w));
    }

    for (auto& w : workers) {
        w.join();
    }
    return 0;
}
