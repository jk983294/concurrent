#ifndef _ZERG_UNIX_H_
#define _ZERG_UNIX_H_

#include <pthread.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctime>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include "zerg_util.h"

namespace ztool {

inline void Escape(void *p) { asm volatile("" : : "g"(p) : "memory"); }

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

inline bool CheckProcessAlive(pid_t id) {
    if (id <= 0) return false;
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
    }  // Wait for defunct, zombie sub-process get collected
    return 0 == kill(id, 0);
}

inline void MemUsage(double &vm_usage, double &resident_set) {
    using std::ifstream;
    using std::ios_base;
    using std::string;

    vm_usage = 0.0;
    resident_set = 0.0;
    ifstream stat_stream("/proc/self/stat", ios_base::in);
    string pid, comm, state, parentPid, procGroup, session, tty;
    string terminalGroupId, flags, minflt, cminflt, majflt, cmajflt;
    string userTime, systemTime, childUserTime, childSystemTime, priority, nice;
    string numThreads, itrealvalue, starttime;

    unsigned long vsize;  // virtual memory size in bytes
    long rss;             // resident set size: number of pages the process has in real memory

    stat_stream >> pid >> comm >> state >> parentPid >> procGroup >> session >> tty >> terminalGroupId >> flags >>
        minflt >> cminflt >> majflt >> cmajflt >> userTime >> systemTime >> childUserTime >> childSystemTime >>
        priority >> nice >> numThreads >> itrealvalue >> starttime >> vsize >> rss;  // don't care about the rest

    stat_stream.close();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;  // in case x86-64 is configured to use 2MB pages
    vm_usage = vsize / 1024.0;
    resident_set = rss * page_size_kb;
}

inline std::vector<size_t> GetAffinity(pthread_t id = 0) {
    std::vector<size_t> ret;
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    if (id == 0) id = pthread_self();
    auto s = pthread_getaffinity_np(id, sizeof(cpu_set_t), &cpuSet);
    (void)s;
    for (size_t j = 0; j < CPU_SETSIZE; j++) {
        if (CPU_ISSET(j, &cpuSet)) ret.push_back(j);
    }
    return ret;
}

inline std::string GetHostName() {
    std::string hostname(1024, '\0');
    gethostname((char *)hostname.data(), hostname.capacity());

    auto pos = hostname.find('.');
    if (pos != std::string::npos) {
        return hostname.substr(0, pos);
    } else
        return hostname;
}

inline std::string GetDomainName() {
    std::string hostname(1024, '\0');
    gethostname((char *)hostname.data(), hostname.capacity());

    auto pos = hostname.find('.');
    if (pos != std::string::npos) {
        return hostname.substr(pos + 1, hostname.size() - pos);
    } else
        return "";
}

struct zerg_system {
    std::string program;
    int process;
    std::string xml;
    std::string ID;
    std::string user;
    long start_microsecond;
    std::string log_file;
    std::string cmd;
    std::string comm;
    ztool::ZergTimezone timezone;

    void Init() {
        struct timespec start;
        clock_gettime(CLOCK_REALTIME, &start);
        start_microsecond = start.tv_sec * 1000 * 1000 + start.tv_nsec / 1000;
        process = getpid();
        cmd = GetProgramPath();
        user = GetUserName();
        comm = GetProcCmdline(process);
        timezone = GetZergTimezone();
    }
};

}  // namespace ztool

#endif
