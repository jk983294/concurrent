#ifndef CONCURRENT_MONITOR_H
#define CONCURRENT_MONITOR_H

#include <mutex>

namespace frenzy {
/**
 * Monitor is very like synchronized(monitor_object) {} in Java world
 */
template <typename T>
class Monitor {
private:
    T t;
    mutable std::mutex mtx;

public:
    Monitor(T t_ = T{}) : t(t_) {}

    template <typename Functor>
    auto operator()(Functor f) const -> decltype(f(t)) {
        std::lock_guard<std::mutex> _{mtx};
        return f(t);
    }
};
}

#endif
