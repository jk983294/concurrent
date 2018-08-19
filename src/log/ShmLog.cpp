#include "ShmLog.h"
#include <sys/mman.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <cstdarg>
#include "media/ShmUtils.h"

using namespace std;

namespace frenzy {

    bool ShmLog::initShm(const string& logShmName, int shmEntryCount) {
        static bool result = false;
        if (shmInited) {
            cerr << "ShmLog::initShm already called, return last result:" << result;
            return result;
        }
        shmInited = true;

        if (logShmName == "") {
            cout << "empty logShmName, print to console" << endl;
            result = true;
            return true;
        }

        shmPath = logShmName;
        maxCount = shmEntryCount;

        size_t fileSize = maxCount * sizeof(ShmContent);
        printf("shm opened with %d * %zu = %zd\n", maxCount, sizeof(ShmContent), fileSize);
        pshm = (char*)create_mmap_with_meta(logShmName, fileSize);

        if (pshm == nullptr) {
            perror("mmap");
            return false;
        }

        pMeta = (ShmLogMeta*)pshm;
        ShmLogMeta dummy;
        dummy.totalSize = fileSize;
        dummy.contentCount = maxCount;
        *pMeta = dummy;

        pData = (ShmContent*)(pshm + META_SIZE);
        pIndex = &pMeta->writeIndex;
        dumpThread = std::thread([this] { dumpLog(); });
        result = true;
        return true;
    }

    bool ShmLog::open(const string& outfileName, ShmLogPriority priority) {
        if (opened_) {
            cerr << "ShmLog::open called multi times" << endl;
            return false;
        }
        priority_ = priority;
        opened_ = true;
        cout << "ShmLog::open, outfile name:" << outfileName << endl;

        if (pMeta == nullptr || shmPath == "") {
            return true;
        } else {
            strcpy(pMeta->filePath, outfileName.c_str());
            if (ofs != nullptr) {
                cerr << "ShmLog::open failed, it cannot bind to two outputs" << endl;
                return false;
            }

            ofs = new ofstream;
            ofs->open(outfileName, ios::out | ios::app);
            if (!ofs->is_open()) {
                cerr << "open outfile:" << outfileName << " failed" << endl;
                return false;
            }
            os = ofs;
        }
        return true;
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
                // shmindex exceeded fileout index
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

        int n = sprintf(buf, "[%s:%d]", sourceFile, line);
        buf += n;

        va_list argp;
        va_start(argp, formatStr);
        if (-1 == vsnprintf(buf, MaxLogLength, formatStr, argp)) buf[MaxLogLength - 1] = 0;
        va_end(argp);
    }

}