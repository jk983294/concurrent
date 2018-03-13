#ifndef CONCURRENT_CONCURRENT_WRAPPER_H
#define CONCURRENT_CONCURRENT_WRAPPER_H

#include <future>
#include <thread>
#include "ConcurrentQueue.h"

namespace frenzy {

namespace {
template <typename Future, typename Functor, typename T>
void promise_set_value(std::promise<Future>& prom, Functor& f, T& t) {
    prom.set_value(f(t));
}

template <typename Future, typename T>
void promise_set_value(std::promise<void>& prom, Future& f, T& t) {
    f(t);
    prom.set_value();
}
}

template <typename T>
class ConcurrentWrapper {
private:
    mutable ConcurrentQueue<std::function<void()>> queue_;
    T resource_;
    std::thread worker_;
    bool done_;

public:
    ConcurrentWrapper(T resource = T{}) : resource_{resource}, done_{false} {
        worker_ = std::thread([this] {
            while (!done_) queue_.pop()();
        });
    }

    ~ConcurrentWrapper() {
        // enqueue last task to set done_ to true to shutdown worker_ thread
        queue_.push([this] { done_ = true; });
        worker_.join();
    }

    template <typename Functor>
    auto submit(Functor f) const -> std::future<decltype(f(resource_))> {
        auto prom = std::make_shared<std::promise<decltype(f(resource_))>>();
        auto fut = prom->get_future();
        queue_.push([=] {
            try {
                promise_set_value(*prom, f, resource_);
            } catch (std::exception&) {
                prom->set_exception(std::current_exception());
            }
        });
        return fut;
    }

    template <typename Functor>
    void operator()(Functor f) const {
        queue_.push([=] { f(resource_); });
    }
};
}

#endif
