#ifndef CONCURRENT_ASYNC_LOG_H
#define CONCURRENT_ASYNC_LOG_H

#include <iostream>
#include <sstream>
#include "ConcurrentWrapper.h"
#include "Utils.h"

namespace frenzy {

class AsyncLog {
public:
    // lazy initialization, C++11 does guarantee that this is thread-safe
    static AsyncLog &instance() {
        static AsyncLog *instance = new AsyncLog(std::cout);
        // return reference to removes temptation to try and delete the returned instance.
        return *instance;
    }

private:
    AsyncLog(std::ostream &o) : log(o) {}

    // this prevents accidental copying of the only instance of the class.
    AsyncLog(const AsyncLog &old);
    const AsyncLog &operator=(const AsyncLog &old);

    // This prevents others from deleting our one single instance, which was otherwise created on the heap
    ~AsyncLog() {}

public:
    ConcurrentWrapper<std::ostream &> log;
};

#define PERFORM_LOG(file, line, content)                                                      \
    {                                                                                         \
        std::ostringstream oss;                                                               \
        oss << frenzy::time_string() << " " << file << ":" << line << " " << content << "\n"; \
        std::string __result = oss.str();                                                     \
        frenzy::AsyncLog::instance().log([__result](ostream &c) { c << __result; });          \
    }

#define ASYNC_LOG(str) PERFORM_LOG(__FILE__, __LINE__, str)
}

#endif
