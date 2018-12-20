#include "AsioLog.h"
#include "utils/Utils.h"

#define ASIO_LOG_PRIORITY_STRING_ERROR "ERROR"
#define ASIO_LOG_PRIORITY_STRING_WARNING "WARNING"
#define ASIO_LOG_PRIORITY_STRING_INFO "INFO"
#define ASIO_LOG_PRIORITY_STRING_DEBUG "DEBUG"

namespace frenzy {
static const char* to_string(LogPriority priority) {
    switch (priority) {
        case LogPriority::DEBUG:
            return ASIO_LOG_PRIORITY_STRING_DEBUG;
        case LogPriority::INFO:
            return ASIO_LOG_PRIORITY_STRING_INFO;
        case LogPriority::WARNING:
            return ASIO_LOG_PRIORITY_STRING_WARNING;
        case LogPriority::ERROR:
            return ASIO_LOG_PRIORITY_STRING_ERROR;
        default:
            return "unknown";
    }
}

// if not found, return default INFO
static LogPriority from_string_log_priority(const char* str) {
    for (int i = LogPriority::DEBUG; i <= LogPriority::ERROR; i++) {
        if (strcmp(str, to_string(static_cast<LogPriority>(i))) == 0) return static_cast<LogPriority>(i);
    }
    return INFO;
}

LogQueueItem::LogQueueItem(LogPriority p, const char* f, int l, const std::string& m, const timespec& lt, size_t count,
                           int tid_)
    : priority(p), file(f), line(l), msg(m), logTime(lt), repeatCount(count), tid(tid_) {}

LogQueueItem::LogQueueItem(const std::shared_ptr<std::promise<void>>& flushPromise) : flushPromise(flushPromise) {}

void LogQueueItem::set(LogPriority p, const char* f, int l, const std::string& m, const timespec& lt, size_t count,
                       int tid_) {
    priority = p;
    file = f;
    line = l;
    msg = m;
    logTime = lt;
    repeatCount = count;
    tid = tid_;
}

bool LogQueueItem::equals(LogPriority p, const char* f, int l, const std::string& m, int tid_) const {
    return priority == p && file == f && line == l && msg == m && tid == tid_;
}

void LogWriter::write_log(LogPriority priority, const char* file, int line, const std::string& msg) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    int tid = syscall(__NR_gettid);

    std::lock_guard<std::mutex> lock(mtx);
    if (lastItem.equals(priority, file, line, msg, tid)) {
        lastItem.update_duplicate(now);  // if dup, then no write util last dup
        return;
    }

    if (lastItem.repeatCount) {
        dropped += q.try_push(LogQueueItem(lastItem.priority, lastItem.file.c_str(), lastItem.line, lastItem.msg,
                                           lastItem.logTime, lastItem.repeatCount, lastItem.tid));
    }
    lastItem.set(priority, file, line, msg, now, 0, tid);
    dropped += q.try_push(LogQueueItem(lastItem.priority, lastItem.file.c_str(), lastItem.line, lastItem.msg,
                                       lastItem.logTime, lastItem.repeatCount, lastItem.tid));
}

void LogWriter::flush_log() {
    std::lock_guard<std::mutex> lock(mtx);
    std::shared_ptr<std::promise<void>> flushPromise(std::make_shared<std::promise<void>>());
    auto future = flushPromise->get_future();
    q.try_push(LogQueueItem(flushPromise));
    future.wait_for(flushTimeout);
}

void LogWriter::on_batch_timeout(const boost::system::error_code& error) {
    if (!error) {
        process_queue(batchSize);
    } else {
        std::clog << "error in LogWriter<Output, Format, ASYNC>::on_batch_timeout " << error << '\n';
    }

    if (isRunning) {
        batchTimer.expires_from_now(boost::posix_time::milliseconds(batchInterval.load()));
        batchTimer.async_wait(boost::bind(&LogWriter::on_batch_timeout, this, boost::asio::placeholders::error));
    }
}

void LogWriter::process_queue(size_t size) {
    for (size_t i = 0; i < size; i++) {
        LogQueueItem item;
        if (q.try_pop(item)) {
            std::shared_ptr<std::promise<void>>& flushPromise = item.flushPromise;
            if (flushPromise) {
                flushPromise->set_value();
            } else {
                item.msg = format(item);
                write(item);
            }
        } else {
            break;
        }
    }
}

void LogWriter::service() {
    try {
        keepAliveTimer.expires_from_now(boost::posix_time::pos_infin);
        keepAliveTimer.async_wait(boost::bind(&LogWriter::keep_alive, this, boost::asio::placeholders::error));

        batchTimer.expires_from_now(boost::posix_time::milliseconds(batchInterval.load()));
        batchTimer.async_wait(boost::bind(&LogWriter::on_batch_timeout, this, boost::asio::placeholders::error));

        while (isRunning) {
            iosvc.run();
        }
    } catch (...) {
        std::clog << "LogWriter unhandled exception in service loop " << '\n';
    }
}

std::string LogWriter::format(const LogQueueItem& data) {
    ++seqNo;
    std::ostringstream os;
    os << timespec2string(data.logTime) << " [" << pid << ":" << data.tid << "] " << seqNo << " "
       << to_string(data.priority) << " " << data.msg << " (" << data.file << ":" << data.line << ")";

    if (data.repeatCount) {
        os << " [repeated " << data.repeatCount << " times]";
    }
    return os.str();
}

void Log::set_priority(const std::string& str) { priority = from_string_log_priority(str.c_str()); }
}  // namespace frenzy
