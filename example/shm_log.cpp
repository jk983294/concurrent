#include <log/ShmLog.h>
#include <atomic>
#include <functional>
#include <vector>

using namespace std;

std::atomic<int> value;

void worker(int id) {
    while (1) {
        int localValue = value.load();
        SHM_LOG_INFO("thread %d get value %d", id, localValue);
        ++value;
        std::this_thread::sleep_for(std::chrono::milliseconds(100 * (rand() % 5)));
    }
}

int main() {
    frenzy::ShmLog::instance().initShm("test.log.shm", 128);
    frenzy::ShmLog::instance().open("/tmp/test.log", frenzy::SLP_INFO);

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
