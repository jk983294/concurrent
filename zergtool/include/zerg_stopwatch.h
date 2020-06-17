#ifndef _ZERG_STOPWATCH_H_
#define _ZERG_STOPWATCH_H_

#include <sys/time.h>
#include <ctime>
#include <exception>

namespace ztool {

class StopWatch {
public:
    StopWatch() = default;
    virtual ~StopWatch() = default;

    void Start() { start(); }
    void start() { clock_gettime(CLOCK_REALTIME, &start_ts); }

    void Stop() { stop(); }
    void stop() { clock_gettime(CLOCK_REALTIME, &end_ts); }

    // return time duration between start and end
    long print() {
        elapse = (end_ts.tv_sec - start_ts.tv_sec) * 1000 * 1000 * 1000;
        elapse += end_ts.tv_nsec - start_ts.tv_nsec;
        return elapse;
    }

private:
    timespec start_ts, end_ts;  // store every time point data
    long elapse;                // store time elapsed during start and stop
};

}  // namespace ztool

#endif
