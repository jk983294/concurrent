#ifndef CONCURRENT_ASIO_LOG_H
#define CONCURRENT_ASIO_LOG_H

#include <tbb/concurrent_queue.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <ctime>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include "utils/Singleton.h"

namespace frenzy {

enum LogPriority { DEBUG, INFO, WARNING, ERROR };

class LogMsg {
    std::ostringstream oss;

public:
    typedef LogMsg& (*LogMsgFunctor)(LogMsg&);
    typedef std::basic_ostream<char, std::char_traits<char>> StdOutStream;
    typedef StdOutStream& (*StdFunctor)(StdOutStream&);

    ~LogMsg() {}
    LogMsg() {}

    LogMsg(const std::string&& msg) { oss << msg; }
    LogMsg(const std::string& msg) { oss << msg; }
    const std::string str() const { return oss.str(); }

    template <class T>
    LogMsg& operator<<(const T& t) {
        oss << t;
        return *this;
    }

    LogMsg& operator<<(LogMsgFunctor f) { return f(*this); }

    // allow msg << endl << alphabool
    LogMsg& operator<<(StdFunctor f) {
        f(oss);
        return *this;
    }
};

class LogQueueItem {
public:
    LogPriority priority;
    std::string file;
    int line = 0;
    std::string msg;
    timespec logTime = {};
    size_t repeatCount = 0;
    int tid = 0;
    std::shared_ptr<std::promise<void>> flushPromise;

public:
    LogQueueItem(LogPriority p, const char* f, int l, const std::string& m, const timespec& lt, size_t count, int tid_);

    explicit LogQueueItem(
        const std::shared_ptr<std::promise<void>>& flushPromise = std::shared_ptr<std::promise<void>>());

    void set(LogPriority p, const char* f, int l, const std::string& m, const timespec& lt, size_t count, int tid_);

    void update_duplicate(const timespec& lt) {
        ++repeatCount;
        logTime = lt;
    }

    bool equals(LogPriority p, const char* f, int l, const std::string& m, int tid_) const;

    const std::string& message() const { return msg; }
};

class LogWriter {
private:
    boost::asio::io_service iosvc;
    boost::asio::deadline_timer keepAliveTimer;
    boost::asio::deadline_timer batchTimer;
    tbb::concurrent_bounded_queue<LogQueueItem> q;
    std::atomic<size_t> dropped;
    std::atomic<size_t> batchSize{1024};
    std::atomic<size_t> batchInterval{250};  // ms
    std::atomic<bool> isRunning{false};
    std::thread thread;
    std::chrono::seconds flushTimeout;
    std::mutex mtx;
    LogQueueItem lastItem;
    unsigned seqNo{0};
    pid_t pid{0};

public:
    explicit LogWriter(const std::chrono::seconds& timeout = std::chrono::seconds(5))
        : keepAliveTimer(iosvc), batchTimer(iosvc), flushTimeout(timeout) {
        pid = getpid();
        start();
    };

    ~LogWriter() { stop(); }

    void start() {
        std::lock_guard<std::mutex> lock(mtx);
        if (!thread.joinable()) {
            isRunning = true;
            thread = std::thread(&LogWriter::service, this);
        }
    }

    void stop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (isRunning) {
            isRunning = false;
            iosvc.stop();
            thread.join();
            process_queue(q.size());
        }
    }

    void write_log(LogPriority priority, const char* file, int line, const std::string& msg);
    void flush_log();

    void keep_alive(const boost::system::error_code& error) {
        if (error) {
            std::clog << "LogWriter keep alive timer error " << error << '\n';
        }
    }

    void on_batch_timeout(const boost::system::error_code& error);
    void process_queue(size_t size);
    void service();

    void write(const LogQueueItem& data) { std::cout << data.message() << '\n'; }

    std::string format(const LogQueueItem& data);
};

class Log {
public:
    std::shared_ptr<LogWriter> writer;
    std::atomic<LogPriority> priority;

public:
    ~Log() {}
    static Log& instance() { return Singleton<Log>::instance(); }

    friend class frenzy::Singleton<Log>;

    std::shared_ptr<LogWriter> get_writer() { return writer; }

    bool can_log(LogPriority priority_) { return priority >= priority_; }

    void log(LogPriority priority, const char* file, int line, const LogMsg& msg) {
        writer->write_log(priority, file, line, msg.str());
    }

    void set_priority(const std::string& str);

private:
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    Log() : writer(new LogWriter), priority(INFO) {}
};
}  // namespace frenzy

#define PERFORM_LOG(priority, file, line, str)                                          \
    {                                                                                   \
        if (frenzy::Log::instance().can_log(priority)) {                                \
            frenzy::Log::instance().log(priority, file, line, frenzy::LogMsg() << str); \
        }                                                                               \
    }

#define ASIO_LOG_SET_PRIORITY(priority) frenzy::Log::instance().priority = priority;

#define ASIO_LOG_DEBUG(str) PERFORM_LOG(frenzy::DEBUG, __FILE__, __LINE__, str)
#define ASIO_LOG_INFO(str) PERFORM_LOG(frenzy::INFO, __FILE__, __LINE__, str)
#define ASIO_LOG_WARNING(str) PERFORM_LOG(frenzy::WARNING, __FILE__, __LINE__, str)
#define ASIO_LOG_ERROR(str) PERFORM_LOG(frenzy::ERROR, __FILE__, __LINE__, str)

#define ASIO_LOG_FLUSH() \
    { frenzy::Log::instance().get_writer()->flush_log(); }

#endif
