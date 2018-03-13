#ifndef CONCURRENT_ACTIVE_H
#define CONCURRENT_ACTIVE_H

#include <thread>
#include "ConcurrentQueue.h"

namespace frenzy {

class Active {
private:
    ConcurrentQueue<std::function<void()>> queue_;
    std::thread worker_;
    bool done_;

private:
    Active(const Active&) = delete;
    Active& operator=(const Active&) = delete;

public:
    Active() : done_{false} {
        worker_ = std::thread([this] {
            while (!done_) queue_.pop()();
        });
    }

    ~Active() {
        queue_.push([this] { done_ = true; });
        worker_.join();
    }

    void submit(std::function<void()> callback_) { queue_.push(callback_); }
};
}

#endif
