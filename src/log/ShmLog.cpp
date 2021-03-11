#include "ShmLog.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <utils/Utils.h>
#include <cstdarg>
#include <cstring>
#include <map>
#include "media/ShmUtils.h"

using namespace std;

namespace frenzy {

bool ShmLog::initShm(string logShmName, int shmEntryCount) {
    static bool result = false;
    if (shmInited) {
        cerr << "ShmLog::initShm already called, return last result:" << result;
        return result;
    }
    shmInited = true;

    if (logShmName.empty()) {
        logShmName = "shm_log." + now_string();
    }

    shmPath = logShmName;
    maxCount = shmEntryCount;

    size_t fileSize = static_cast<size_t>(maxCount) * sizeof(ShmContent);
    printf("/dev/shm/%s shm opened with %d * %zu = %zd\n", logShmName.c_str(), maxCount, sizeof(ShmContent), fileSize);
    pShm = (char*)create_mmap_with_meta(logShmName, fileSize);

    if (pShm == nullptr) {
        perror("mmap");
        return false;
    }

    pMeta = (ShmLogMeta*)pShm;
    ShmLogMeta dummy;
    dummy.totalSize = fileSize;
    dummy.contentCount = maxCount;
    *pMeta = dummy;

    pData = (ShmContent*)(pShm + META_SIZE);
    pIndex = &pMeta->writeIndex;
    dumpThread = std::thread([this] { dumpLog(); });
    result = true;
    return true;
}

bool ShmLog::open(string outfileName, ShmLogPriority priority, bool print, bool printSource_) {
    if (!shmInited) {
        initShm();
    }

    if (opened_) {
        cerr << "ShmLog::open called multi times" << endl;
        return false;
    }
    if (outfileName.empty()) {
        outfileName = "/tmp/shm_log." + now_string();
    }
    if (print) {
        outfileName = "std stream";
    }
    priority_ = priority;
    printSource = printSource_;
    opened_ = true;

    cout << "ShmLog::open, outfile name: " << outfileName << endl;

    if (pMeta == nullptr || shmPath.empty()) {
        return true;
    } else {
        strcpy(pMeta->filePath, outfileName.c_str());
        if (ofs != nullptr) {
            cerr << "ShmLog::open failed, it cannot bind to two outputs" << endl;
            return false;
        }

        if (print) {
            os = &std::cout;
        } else {
            ofs = new ofstream;
            ofs->open(outfileName, ios::out | ios::app);
            if (!ofs->is_open()) {
                cerr << "open outfile:" << outfileName << " failed" << endl;
                return false;
            }
            os = ofs;
        }
    }
    return true;
}

ShmLogPriority ShmLog::getPriorityByStr(std::string p) {
    if (p == "debug")
        return frenzy::ShmLogPriority::SLP_DEBUG;
    else if (p == "warning")
        return frenzy::ShmLogPriority::SLP_WARNING;
    else if (p == "error")
        return frenzy::ShmLogPriority::SLP_ERROR;
    else if (p == "critical")
        return frenzy::ShmLogPriority::SLP_CRITICAL;
    else
        return frenzy::ShmLogPriority::SLP_INFO;
}

const int KeepLinesForConcurrency = 10;
void ShmLog::dumpLog() {
    int doneIndex = -1;
    while (true) {
        if (readWarpCount > writeWarpCount) {
            usleep(100 * 1000);
            continue;
        }

        if (writeWarpCount - readWarpCount > 1) {
            cerr << "too fast log case 1, unable to write to disk file, exit dump thread" << endl;
            cerr << "writeWarpCount:" << writeWarpCount << ", readWarpCount:" << readWarpCount << endl;
            break;
        }

        int curIndex = *pIndex;
        int dstIndex = curIndex;
        if (writeWarpCount - readWarpCount == 1) {
            cout << "warp1, doneIndex:" << doneIndex << ", curIndex:" << curIndex << endl;
            // shm index exceeded fileout index
            if (doneIndex < curIndex + 10 * KeepLinesForConcurrency) {
                cerr << "too fast log case 2, unable to write to disk file, exit dump thread" << endl;
                break;
            }

            dstIndex = maxCount - 1;
        }

        // cout << "batch 1, doneIndex:" << doneIndex << ", curIndex:" << curIndex << ", dstIndex:" << dstIndex << endl;
        for (int i = doneIndex + 1; i <= min(dstIndex - KeepLinesForConcurrency, maxCount - 1); ++i) {
            *os << genLogContent(pData[i]) << endl;
            ++doneIndex;
        }

        if (dstIndex != maxCount - 1) {  // not warp
            usleep(1000);
        }

        if (writeWarpCount - readWarpCount > 1) {
            cerr << "too fast log case 3, unable to write to disk file, exit dump thread" << endl;
            cerr << "writeWarpCount:" << writeWarpCount << ", readWarpCount:" << readWarpCount << endl;
            break;
        }
        if (writeWarpCount - readWarpCount == 1) {
            if (doneIndex < *pIndex + 10 * KeepLinesForConcurrency) {
                cerr << "too fast log case 4, unable to write to disk file, exit dump thread" << endl;
                break;
            }

            dstIndex = maxCount - 1;
        }

        for (int i = doneIndex + 1; i <= min(dstIndex, maxCount - 1); ++i) {
            *os << genLogContent(pData[i]) << endl;
            ++doneIndex;
        }

        if (doneIndex >= maxCount - 1) {
            // read finish the last element, warp back
            ++readWarpCount;
            doneIndex = -1;
        }
    }
}

void ShmLog::log(ShmLogPriority priority, const char* sourceFile, int line, const char* formatStr, ...) {
    if (!shmInited) {
        ShmContent shmContent;
        clock_gettime(CLOCK_REALTIME, &shmContent.ts);
        shmContent.priority = priority;
        char* buf = shmContent.msg;

        va_list argp;
        va_start(argp, formatStr);
        int n = vsnprintf(buf, MaxLogLength, formatStr, argp);
        va_end(argp);
        if (-1 == n || !printSource)
            buf[MaxLogLength - 1] = '\0';
        else {
            buf += n;
            sprintf(buf, " (%s:%d)", sourceFile, line);
        }
        cout << genLogContent(shmContent) << endl;
        return;
    }
    int index = -1;
    while ((index = __sync_add_and_fetch(pIndex, 1)) >= maxCount) {
        if (__sync_bool_compare_and_swap(pIndex, maxCount, -1) == true) {
            __sync_add_and_fetch(&writeWarpCount, 1);
        } else if (*pIndex > maxCount) {
            std::cerr << "shm overflow, maybe some concurrent log instance" << std::endl;
            return;
        }
    }

    clock_gettime(CLOCK_REALTIME, &pData[index].ts);
    pData[index].priority = priority;
    char* buf = pData[index].msg;

    va_list argp;
    va_start(argp, formatStr);
    int n = vsnprintf(buf, MaxLogLength, formatStr, argp);
    va_end(argp);
    if (-1 == n || !printSource)
        buf[MaxLogLength - 1] = '\0';
    else {
        buf += n;
        sprintf(buf, " (%s:%d)", sourceFile, line);
    }
}

}  // namespace frenzy
