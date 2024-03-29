#ifndef _ZERG_TOOL_UTIL_H_
#define _ZERG_TOOL_UTIL_H_

#include <sched.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <string>
#include "IniReader.h"
#include "folly_foreach.h"
#include "xml_parser.h"
#include "zerg_exception.h"
#include "zerg_file.h"
#include "zerg_macro.h"
#include "zerg_string.h"
#include "zerg_time.h"

namespace ztool {
class BashCommand {
    const int READ = 0;
    const int WRITE = 1;

public:
    FILE* pFile;
    pid_t pid;
    std::string m_command;
    std::string m_type;

    BashCommand(std::string command, std::string type) {
        m_command = command;
        m_type = type;
    }

    FILE* Run() {
        pid_t child_pid;
        int fd[2];
        pipe(fd);

        if ((child_pid = fork()) == -1) {
            perror("fork");
            exit(1);
        }

        if (child_pid == 0) {  // child
            if (m_type == "r") {
                close(fd[READ]);     // Close the READ end of the pipe since the child's fd is write-only
                dup2(fd[WRITE], 1);  // Redirect stdout to pipe
            } else {
                close(fd[WRITE]);   // Close the WRITE end of the pipe since the child's fd is read-only
                dup2(fd[READ], 0);  // Redirect stdin to pipe
            }

            setpgid(child_pid, child_pid);  // Needed so negative PIDs can kill children of /bin/sh
            execl("/bin/sh", "/bin/sh", "-c", m_command.c_str(), nullptr);
            exit(0);
        } else {
            if (m_type == "r") {
                close(fd[WRITE]);  // Close the WRITE end of the pipe since parent's fd is read-only
            } else {
                close(fd[READ]);  // Close the READ end of the pipe since parent's fd is write-only
            }
        }

        pid = child_pid;
        if (m_type == "r") {
            pFile = fdopen(fd[READ], "r");
            return pFile;
        }
        pFile = fdopen(fd[WRITE], "w");
        return pFile;
    }

    int Shutdown() {
        int stat;
        fclose(pFile);
        kill(pid, SIGKILL);
        while (waitpid(pid, &stat, 0) == -1) {
            if (errno != EINTR) {
                stat = -1;
                break;
            }
        }
        return stat;
    }
};

inline int EnableCoreDump() {
    struct rlimit core_limits;
    core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &core_limits);
    return 0;
}

inline int DisableCoreDump() {
    struct rlimit core_limits;
    core_limits.rlim_cur = core_limits.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &core_limits);
    return 0;
}

struct ZergTime {
    int hour;
    int minute;
    int second;

    long ToInt() { return hour * 100 * 100 * 1000 + minute * 100 * 1000 + second * 1000; }

    ZergTime(int h, int m, int s) {
        hour = h;
        minute = m;
        second = s;
    }
};

struct ZergTimezone {
    char tz[32];
    int utc_offset;  // in minutes
};

inline ZergTimezone GetZergTimezone() {
    ZergTimezone ret;
    XmlNode config("/opt/version/latest/etc/system.xml");
    auto tzconfig = config.getChild("timezone");
    strcpy(ret.tz, tzconfig.getAttrOrThrow("tz").c_str());
    auto offset_str = tzconfig.getAttrOrThrow("offset");
    ret.utc_offset = 60 * std::stoi(offset_str.substr(offset_str.find("GMT") + 3));
    return ret;
}

inline int WriteProgramPid(const std::string& program) {
    pid_t pid = getpid();
    std::ofstream ofs;
    ofs.open(std::string("/opt/run/") + program + std::string(".pid"));
    if (ofs.fail()) THROW_ZERG_EXCEPTION(std::string("WriteProgramPid failed: ") << strerror(errno));
    ofs << pid;
    ofs.close();
    return 0;
}

inline long GetProgramPid(const std::string& program) {
    long pid = 0;
    std::string filename = std::string("/opt/run/") + program + std::string(".pid");
    if (!IsFileExisted(filename)) return pid;
    std::ifstream ofs;
    ofs.open(filename);
    if (ofs.fail()) THROW_ZERG_EXCEPTION(std::string("GetProgramPid failed: ") << strerror(errno));
    ofs >> pid;
    ofs.close();
    return pid;
}

inline bool IsProcessExist(long pid) {
    struct stat sts;
    std::string path = "/proc/" + std::to_string((long long)pid);
    return !(stat(path.c_str(), &sts) == -1 && errno == ENOENT);
}

inline std::string GetProcCmdline(pid_t process) {
    std::string ret;
    unsigned char buffer[4096];
    char filename[128];
    memset(filename, 0, 128);
    sprintf(filename, "/proc/%d/cmdline", process);
    int fd = open(filename, O_RDONLY);
    auto nBytesRead = read(fd, buffer, 4096);
    unsigned char* end = buffer + nBytesRead;
    for (unsigned char* p = buffer; p < end;) {
        if (p == buffer)
            ret += std::string((const char*)p);
        else
            ret += " " + std::string((const char*)p);
        while (*p++)
            ;
    }
    close(fd);
    return ret;
}

inline bool EnsureOneInstance(const std::string& program) {
    int currentPid = getpid();
    long filePid = GetProgramPid(program);
    if (currentPid == filePid) return true;
    if (!IsProcessExist(filePid)) return true;
    string fileCmdLine = GetProcCmdline(filePid);
    string cmdLine = GetProcCmdline(currentPid);
    return cmdLine != fileCmdLine;
}

inline bool EnsureOneInstanceFromRun(const std::string& program) {
    std::string filename = std::string("/opt/run/") + program + std::string(".run");
    if (!IsFileExisted(filename)) return true;
    INIReader reader(filename);
    auto pid = reader.GetIntegerOrThrow(program, "process");
    if (!IsProcessExist(pid)) return true;
    std::string cmdline = reader.GetOrThrow(program, "cmdline");
    auto cmdline_ = GetProcCmdline(pid);
    return !(cmdline_ == cmdline);
}

inline bool EnsureOneInstanceFromXml(const std::string& program, const std::string xml) {
    std::string filename = std::string("/opt/run/") + program + std::string(".run");
    if (!IsFileExisted(filename)) return true;
    INIReader reader(filename);
    auto pid = reader.GetIntegerOrThrow(program, "process");
    if (!IsProcessExist(pid)) return true;
    std::string xmlFile = reader.GetOrThrow(program, "xml");
    std::string realXml = GetAbsolutePath(xml);
    return !(realXml == xmlFile);
}

inline void BindCore(size_t cpu_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) THROW_ZERG_EXCEPTION("CPU affinity setting failed.");
}

inline void BindCore(std::vector<size_t>& cpu_id_list) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (auto node : cpu_id_list) {
        CPU_SET(node, &mask);
    }
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) THROW_ZERG_EXCEPTION("CPU affinity setting failed.");
}

// get cpu cycles counts
uint64_t inline GetRDTSC() {
    unsigned int lo, hi;
    __asm(
        "XOR %eax, %eax\t\n"
        "CPUID\t\n"
        "XOR %eax, %eax\t\n"
        "CPUID\t\n"
        "XOR %eax, %eax\t\n"
        "CPUID\t\n");
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

inline ZergTime StringToTime(const char* str) {
    int hour = 0;
    int minute = 0;
    int second = 0;

    int c = 0, x = 0;
    const char* p = str;
    for (c = *(p++); (c < 48 || c > 57); c = *(p++)) {
        if (c == 45) {
            c = *(p++);
            break;
        }
    };  // eat whitespaces

    int index = 0;
    while (*p != '\0') {
        for (; c > 47 && c < 58; c = *(p++)) x = (x << 1) + (x << 3) + c - 48;
        ++index;
        if (index == 1)
            hour = x;
        else if (index == 2)
            minute = x;
        else if (index == 3)
            second = x;
        else
            break;
        c = *(p++);
        x = 0;
    }
    return {hour, minute, second};
}

inline std::string ReplaceStringHolderCopy(const std::string& str, const std::map<std::string, std::string>& holders) {
    std::string ret = str;
    std::string holder;
    for (const auto& iter : holders) {
        holder = "${" + iter.first + "}";
        ReplaceAll(ret, holder, iter.second);
    }
    return ret;
}

inline std::string ReplaceStringHolder(std::string& str, const std::map<std::string, std::string>& holders) {
    std::string holder;
    for (const auto& iter : holders) {
        holder = "${" + iter.first + "}";
        ReplaceAll(str, holder, iter.second);
    }
    return str;
}

template <typename T>
inline std::string ReplaceSpecialTimeHolderCopy(const std::string& str, const Clock<T>& clock) {
    std::string year, month, day, timestamp, timestampsss;
    year = clock.YearToStr();
    month = clock.MonthToStr();
    day = clock.DayToStr();

    timestamp = clock.HourToStr() + clock.MinuteToStr() + clock.SecondToStr();
    timestampsss = timestamp + clock.MillisecondToStr();

    std::string ret = str;
    ReplaceAll(ret, "${YYYY}", year);
    ReplaceAll(ret, "${MM}", month);
    ReplaceAll(ret, "${DD}", day);
    ReplaceAll(ret, "${YYYYMMDD}", year + month + day);
    ReplaceAll(ret, "${YYYYMM}", year + month);
    ReplaceAll(ret, "${MMDD}", month + day);
    ReplaceAll(ret, "${HHMMSS}", timestamp);
    ReplaceAll(ret, "${HHMMSSmmm}", timestampsss);
    return ret;
}

template <typename T>
inline std::string ReplaceSpecialTimeHolder(std::string& str, const Clock<T>& clock) {
    auto ret = ReplaceSpecialTimeHolderCopy(str, clock);
    str = ret;
    return ret;
}

inline std::string ReplaceSpecialTimeHolderCopy(const std::string& str) {
    Clock<> clock;  // use today as default
    clock.Update();
    std::string year, month, day, timestamp, timestampsss;
    year = clock.YearToStr();
    month = clock.MonthToStr();
    day = clock.DayToStr();

    timestamp = clock.HourToStr() + clock.MinuteToStr() + clock.SecondToStr();
    timestampsss = timestamp + clock.MillisecondToStr();

    std::string ret = str;
    ReplaceAll(ret, "${YYYY}", year);
    ReplaceAll(ret, "${MM}", month);
    ReplaceAll(ret, "${DD}", day);
    ReplaceAll(ret, "${YYYYMMDD}", year + month + day);
    ReplaceAll(ret, "${YYYYMM}", year + month);
    ReplaceAll(ret, "${MMDD}", month + day);
    ReplaceAll(ret, "${HHMMSS}", timestamp);
    ReplaceAll(ret, "${HHMMSSmmm}", timestampsss);
    return ret;
}

inline std::string ReplaceSpecialTimeHolderCopy(const std::string& str, const std::string& datetime,
                                                const std::string& format) {
    Clock<> clock(datetime.c_str(), format.c_str());  // use today as default

    std::string year, month, day, timestamp, timestampsss;
    year = clock.YearToStr();
    month = clock.MonthToStr();
    day = clock.DayToStr();

    timestamp = clock.HourToStr() + clock.MinuteToStr() + clock.SecondToStr();
    timestampsss = timestamp + clock.MillisecondToStr();

    std::string ret = str;
    ReplaceAll(ret, "${YYYY}", year);
    ReplaceAll(ret, "${MM}", month);
    ReplaceAll(ret, "${DD}", day);
    ReplaceAll(ret, "${YYYYMMDD}", year + month + day);
    ReplaceAll(ret, "${YYYYMM}", year + month);
    ReplaceAll(ret, "${MMDD}", month + day);
    ReplaceAll(ret, "${HHMMSS}", timestamp);
    ReplaceAll(ret, "${HHMMSSmmm}", timestampsss);
    return ret;
}

inline std::string ReplaceSpecialTimeHolder(std::string& str) {
    auto ret = ReplaceSpecialTimeHolderCopy(str);
    str = ret;
    return ret;
}

inline std::string ReplaceSpecialTimeHolder(std::string& str, const std::string& datetime, const std::string& format) {
    auto ret = ReplaceSpecialTimeHolderCopy(str, datetime, format);
    str = ret;
    return ret;
}

inline std::string GetUserName() {
    char name[200];
    getlogin_r(name, 199);
    return std::string(name);
}

inline std::string GetProgramPath() {
    char name[256];
    auto r = readlink("/proc/self/exe", name, sizeof(name));
    if (r != -1) name[r] = '\0';
    return std::string(name);
}

inline string ReplaceXmlPlaceholder(const string& xml_file, const string& section = "constdefine") {
    auto temp_cfg = XmlNode(xml_file);
    std::map<std::string, std::string> sub_string_holders;
    if (temp_cfg.hasChild(section)) {
        XmlNode const_cfg = temp_cfg.getChild(section);
        auto config_map = const_cfg.getAllAttrMap();
        for (auto& iter : config_map) {
            sub_string_holders[iter.first] = iter.second;
        }
    }

    std::ifstream in(xml_file);
    std::string whole_file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ztool::ReplaceStringHolder(whole_file, sub_string_holders);
    return whole_file;
}

inline void WalkToInclude(ztool::XmlNode& root, ztool::XmlNode& cfg) {
    if (cfg.hasChild("")) {
        for (auto& child : cfg.getChildren()) {
            if (child.getNodeName() == "xml_file") {
                std::string xml_file = child.getAttrOrThrow("file");
                string whole_file = ReplaceXmlPlaceholder(xml_file);
                std::istringstream ss(whole_file);
                ztool::XmlNode subconfig(ss);
                for (auto& iter : subconfig.getChildren()) {
                    cfg.appendChild(iter, root);
                }
            } else
                WalkToInclude(root, child);
        }
    }
}
}  // namespace ztool

#endif
