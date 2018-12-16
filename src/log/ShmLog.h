#ifndef CONCURRENT_SHMLOG_H
#define CONCURRENT_SHMLOG_H

#include <utils/Singleton.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace frenzy {

constexpr int MaxLogLength = 256;
constexpr int DefaultShmLogEntryCount = 2 * 1024 * 1024;
enum ShmLogPriority : uint8_t { SLP_DEBUG = 01, SLP_INFO = 02, SLP_WARNING = 03, SLP_ERROR = 04, SLP_CRITICAL = 05 };

struct __attribute__((__packed__)) ShmContent {
    struct timespec ts;
    ShmLogPriority priority;
    char msg[MaxLogLength];
};

struct ShmLogMeta {
    uint32_t magic{0xFF00EE11};
    size_t metaSize{sizeof(ShmLogMeta)};
    size_t totalSize{0};
    int contentCount{0};
    int writeIndex{0};
    char filePath[256]{0};
};

class ShmLog {
public:
    ShmLog() {}

    ~ShmLog() {
        if (ofs != nullptr) delete ofs;
        if (dumpThread.joinable()) {
            dumpThread.join();
        }
    }

    static ShmLog& instance() { return Singleton<ShmLog>::instance(); }

    /**
     * log count can not be too small. otherwise it is easily entry overflow
     */
    bool initShm(const std::string& logShmName, int maxLogCount = DefaultShmLogEntryCount);

    bool open(const std::string& outfileName = "", ShmLogPriority priority = SLP_INFO);

    ShmLogPriority priority() const { return priority_; }

    void log(ShmLogPriority priority, const char* sourceFile, int line, const char* formatStr, ...);

    void log(ShmLogPriority priority, const char* sourceFile, int line, std::string formatStr, ...) {
        log(priority, sourceFile, line, formatStr.c_str());
    }

    static std::string GetTimeStr(const struct timespec& ts) {
        char buf[32];
        struct tm tmp;
        localtime_r(&(ts.tv_sec), &tmp);
        sprintf(buf, "[%04d%02d%02d-%02d:%02d:%02d.%06d]", tmp.tm_year + 1900, tmp.tm_mon + 1, tmp.tm_mday, tmp.tm_hour,
                tmp.tm_min, tmp.tm_sec, (int)(ts.tv_nsec / 1000));
        return buf;
    }

    static std::string getPriorityStr(ShmLogPriority priority) {
        if (priority == SLP_DEBUG) {
            return "[debug]";
        } else if (priority == SLP_INFO) {
            return "[info]";
        } else if (priority == SLP_WARNING) {
            return "[warning]";
        } else if (priority == SLP_ERROR) {
            return "[error]";
        } else {
            return "[critical]";
        }
    }

    bool can_log(ShmLogPriority priority) { return priority >= priority_; }

private:
    void dumpLog();

    static std::string genLogContent(const ShmContent& shmContent) {
        char buf[1024];
        std::string levelStr = ShmLog::getPriorityStr(shmContent.priority);
        sprintf(buf, "%s%s%s", GetTimeStr(shmContent.ts).c_str(), levelStr.c_str(), shmContent.msg);
        return buf;
    }

public:
    char* pShm{nullptr};  // start address of shm
    ShmLogMeta* pMeta;
    ShmContent* pData;  // ShmContent address
    int* volatile pIndex{nullptr};
    int maxCount{0};
    volatile int writeWarpCount{0};  // current writer pos warp count
    int readWarpCount{0};            // read pos warp count
    ShmLogPriority priority_{SLP_INFO};
    std::string shmPath;
    std::ostream* os{&std::cout};  // no need to destruct
    std::ofstream* ofs{nullptr};   // this should be destructed
    bool shmInited{false};
    bool opened_{false};  // prevent open shm multi time

    std::thread dumpThread;
};

}  // namespace frenzy

#define SHM_PERFORM_LOG(priority, file, line, format, ...)                               \
    {                                                                                    \
        if (frenzy::ShmLog::instance().can_log(priority)) {                              \
            frenzy::ShmLog::instance().log(priority, file, line, format, ##__VA_ARGS__); \
        }                                                                                \
    }

#define SHM_LOG_DEBUG(args...) SHM_PERFORM_LOG(frenzy::ShmLogPriority::SLP_DEBUG, __FILE__, __LINE__, args)
#define SHM_LOG_INFO(args...) SHM_PERFORM_LOG(frenzy::ShmLogPriority::SLP_INFO, __FILE__, __LINE__, args)
#define SHM_LOG_WARNING(args...) SHM_PERFORM_LOG(frenzy::ShmLogPriority::SLP_WARNING, __FILE__, __LINE__, args)
#define SHM_LOG_ERROR(args...) SHM_PERFORM_LOG(frenzy::ShmLogPriority::SLP_ERROR, __FILE__, __LINE__, args)
#define SHM_LOG_CRITICAL(args...) SHM_PERFORM_LOG(frenzy::ShmLogPriority::SLP_CRITICAL, __FILE__, __LINE__, args)

#endif
