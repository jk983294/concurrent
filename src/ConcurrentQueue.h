#ifndef CONCURRENT_CONCURRENT_QUEUE_H
#define CONCURRENT_CONCURRENT_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

namespace frenzy {
template <typename T>
class ConcurrentQueue {
public:
    ConcurrentQueue() = default;
    ConcurrentQueue(const ConcurrentQueue&) = delete;             // disable copying
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;  // disable assignment

    /**
     * if copy constructor throws, then it fails exception safe
     * std::queue use two phase to address, first call front() to get value, then call pop_front()
     */
    T pop() {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
            cond_.wait(mlock);
        }
        auto val = queue_.front();
        queue_.pop();
        return val;
    }

    void pop(T& item) {  // this is exception safe but not natural way to use for client code
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop();
    }

    void push(const T& item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};
}

#endif
