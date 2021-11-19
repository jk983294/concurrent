#ifndef CONCURRENT_TIMERWHEEL_H
#define CONCURRENT_TIMERWHEEL_H

#include <lock/SpinLock.h>
#include <cstdint>

namespace frenzy {
template <typename TTime = int64_t>
class TimerWheel {
public:
    typedef std::function<void(TTime)> TFunc;

    bool advance(TTime now) {
        TFunc func;
        while (true) {
            TTime tm = 0;

            {
                std::lock_guard<frenzy::spin_mutex> lock(mutex_);
                if (!timers_.empty()) {
                    auto& timer = timers_.top();

                    if (timer.tm > now) {
                        return false;
                    } else {
                        tm = timer.tm;
                        func = std::move(timer.func);
                        timers_.pop();
                    }
                } else {
                    return false;
                }
            }
            func(tm);
            return true;
        }
    }

    void register_timer(const TFunc& func, TTime start_time, TTime interval = 0, TTime end_time = 0,
                        TTime black_out_start = 0, TTime black_out_end = 0) {
        std::lock_guard<frenzy::spin_mutex> lock(mutex_);
        if (interval && end_time) {
            for (TTime tm = start_time; tm <= end_time; tm += interval) {
                if (tm >= black_out_start && tm <= black_out_end) {
                } else {
                    timers_.push(TimerScheduler{tm, func});
                }
            }
        } else {
            timers_.push(TimerScheduler{start_time, func});
        }
    }

private:
    struct TimerScheduler {
        TTime tm;
        TFunc func;
    };

    struct Comp {
        bool operator()(const TimerScheduler& a, const TimerScheduler& b) const { return a.tm > b.tm; }
    };

    std::priority_queue<TimerScheduler, std::vector<TimerScheduler>, Comp> timers_;
    frenzy::spin_mutex mutex_;
};
}  // namespace frenzy

#endif
