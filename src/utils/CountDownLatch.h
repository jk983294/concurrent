#ifndef CONCURRENT_COUNT_DOWN_LATCH_H
#define CONCURRENT_COUNT_DOWN_LATCH_H

#include <condition_variable>
#include <mutex>

namespace frenzy {

class CountDownLatch {
public:
    explicit CountDownLatch(int count) : mutex_(), count_(count) {}

    CountDownLatch(CountDownLatch& c) = delete;

    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return count_ == 0; });
        lock.unlock();
    }

    void countDown() {
        std::lock_guard<std::mutex> lock(mutex_);
        --count_;
        if (count_ == 0) {
            condition_.notify_all();
        }
    }

    int getCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    int count_;
};
}

#endif
