#ifndef _ZERG_FILE_H_
#define _ZERG_FILE_H_

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "zerg_clock.h"
#include "zerg_exception.h"

namespace ztool {
struct OutputColumnOption {
    int type{0};  // 1: double, 2:float, 3:int, 4:string, 5:bool
    void* data{nullptr};
    std::string name;
};

struct InputData { // Column-wise data
    InputData() = default;
    ~InputData() {
        clear();
        for (auto* vec : m_double_pool) delete vec;
        for (auto* vec : m_float_pool) delete vec;
        for (auto* vec : m_int_pool) delete vec;
        for (auto* vec : m_bool_pool) delete vec;
        for (auto* vec : m_str_pool) delete vec;
    }

    void clear() {
        m_double_pool.insert(m_double_pool.end(), doubles.begin(), doubles.end());
        m_float_pool.insert(m_float_pool.end(), floats.begin(), floats.end());
        m_int_pool.insert(m_int_pool.end(), ints.begin(), ints.end());
        m_bool_pool.insert(m_bool_pool.end(), bools.begin(), bools.end());
        m_str_pool.insert(m_str_pool.end(), strs.begin(), strs.end());
        doubles.clear();
        ints.clear();
        bools.clear();
        strs.clear();
        cols.clear();
        rows = 0;
    }

    template<typename T>
    std::vector<T>* new_type_vec(std::vector<std::vector<T>*>& pool, std::vector<std::vector<T>*>& pool1, uint64_t size) {
        if (pool.empty()) {
            auto ptr = new std::vector<T>(size);
            pool1.push_back(ptr);
            return ptr;
        } else {
            auto* pFeature = pool.back();
            pool.pop_back();
            pool1.push_back(pFeature);
            pFeature->clear();
            pFeature->resize(size);
            return pFeature;
        }
    }

    std::vector<double>* new_double_vec(uint64_t size) { return new_type_vec<double>(m_double_pool, doubles, size); }
    std::vector<float>* new_float_vec(uint64_t size) { return new_type_vec<float>(m_float_pool, floats, size); }
    std::vector<int>* new_int_vec(uint64_t size) { return new_type_vec<int>(m_int_pool, ints, size); }
    std::vector<bool>* new_bool_vec(uint64_t size) { return new_type_vec<bool>(m_bool_pool, bools, size); }
    std::vector<std::string>* new_str_vec(uint64_t size) { return new_type_vec<std::string>(m_str_pool, strs, size); }

    std::vector<std::vector<int>*> ints;
    std::vector<std::vector<double>*> doubles;
    std::vector<std::vector<float>*> floats;
    std::vector<std::vector<bool>*> bools;
    std::vector<std::vector<std::string>*> strs;
    std::vector<OutputColumnOption> cols;
    uint64_t rows{0};

    std::vector<std::vector<int>*> m_int_pool;
    std::vector<std::vector<double>*> m_double_pool;
    std::vector<std::vector<float>*> m_float_pool;
    std::vector<std::vector<bool>*> m_bool_pool;
    std::vector<std::vector<std::string>*> m_str_pool;
};

/**
 * c++ version of mkdir -p command
 * example: mkdirp("/tmp/dd/aaa", 0755);
 * @param s path
 * @param mode new directory mode
 * @return flags
 */
inline int mkdirp(std::string s, mode_t mode) {
    size_t pre = 0, pos;
    int ret = -1;

    if (s[s.size() - 1] != '/') {
        s = s + std::string("/");  // add trailing / if not found
    }
    while ((pos = s.find_first_of('/', pre)) != std::string::npos) {
        std::string dir = s.substr(0, pos++);
        pre = pos;
        if (dir.empty()) continue;  // if leading / first time is 0 length
        if ((ret = mkdir(dir.c_str(), mode)) && errno != EEXIST) {
            return ret;
        }
    }

    if (ret && errno != EEXIST)
        return ret;
    else
        return 0;
}

/**
 * check if file exist
 * @param name ull path of file
 * @return
 */
inline bool IsFileExisted(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

inline int ChangeFileMode(const std::string& path, mode_t mode) { return chmod(path.c_str(), mode); }

inline int LockFile(const std::string& locker) {
    if (!IsFileExisted(locker)) {
        std::ofstream of(locker);
        of.close();
    }
    ChangeFileMode(locker, 0777);
    const char* file = locker.c_str();
    int fd;
    struct flock lock;
    // Open a file descriptor to the file
    fd = open(file, O_WRONLY);
    // Initialize the flock structure
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    // Place a write lock on the file
    fcntl(fd, F_SETLKW, &lock);
    return fd;
}

inline void UnlockFile(int fd) {
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    // Release the lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
}

/**
 * join paths on linux
 * path_join("/tmp", "aa", "", "bb/", "cc"); // => /tmp/aa/bb/cc
 */
template <class... Args>
inline std::string path_join(const std::string& path, const Args... args) {
    std::array<std::string, sizeof...(args)> name_list = {{args...}};
    std::string ret = path;
    for (size_t i = 0; i < name_list.size(); ++i) {
        if (!ret.empty())
            if (*ret.rbegin() != '/') ret += "/";
        if (name_list[i].empty()) continue;
        ret += name_list[i];
    }
    return ret;
}

inline std::vector<std::string> ListDir(const std::string& directory) {
    auto dir = opendir(directory.c_str());
    std::vector<std::string> ret;
    if (nullptr == dir) {
        return ret;
    }

    auto entity = readdir(dir);
    while (entity != nullptr) {
        if (entity->d_name[0] == '.' &&
            (entity->d_name[1] == '\0' || (entity->d_name[1] == '.' && entity->d_name[2] == '\0'))) {
            entity = readdir(dir);
            continue;
        }
        auto node = path_join(directory, entity->d_name);
        ret.push_back(node);
        entity = readdir(dir);
    }
    closedir(dir);
    return ret;
}

inline std::string FileExpandUser(const std::string& name) {
    if (name.empty() || name[0] != '~') return name;
    char user[128];
    getlogin_r(user, 127);
    return std::string{"/home/"} + user + name.substr(1);
}

inline std::string GetAbsolutePath(const std::string& rawPath) {
    if (rawPath.empty() || rawPath == ".") {
        char* pwd = getcwd(nullptr, 0);
        std::string ret = pwd;
        free(pwd);
        return ret;
    } else if (rawPath[0] == '~') {
        return FileExpandUser(rawPath);
    } else if (rawPath[0] != '/') {
        char* pwd = getcwd(nullptr, 0);
        std::string cur = pwd;
        free(pwd);
        if (rawPath.find("./") == 0)
            return cur + rawPath.substr(1);
        else
            return cur + '/' + rawPath;
    }
    return rawPath;
}

inline bool IsFileReadable(const std::string& filename) { return (access(filename.c_str(), R_OK) != -1); }

inline bool IsFileWritable(const std::string& filename) { return (access(filename.c_str(), W_OK) != -1); }

inline FILE* AssertOpenFile(const std::string& filename, const char* mode) {
    FILE* fp = fopen(filename.c_str(), mode);
    if (fp == nullptr) THROW_ZERG_EXCEPTION("cannot open/create " << filename);
    return fp;
}

inline FILE* AssertFileReadable(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "r");
    if (fp == nullptr) THROW_ZERG_EXCEPTION("cannot open " << filename);
    return fp;
}

// check if the file descriptor is unlinked
inline int IsFdDeleted(int fd) {
    struct stat _stat;
    int ret = -1;
    auto code = fcntl(fd, F_GETFL);
    if (code != -1) {
        if (!fstat(fd, &_stat)) {
            std::cout << _stat.st_nlink << std::endl;
            if (_stat.st_nlink >= 1)
                ret = 0;
            else
                ret = -1;  // deleted
        }
    }
    if (errno != 0) perror("IsFdDeleted");
    return ret;
}

inline std::string Dirname(const std::string& fullname) {
    char p[256];
    strncpy(p, fullname.c_str(), sizeof(p) - 1);
    return string{dirname(p)};
}

inline std::string Basename(const std::string& fullname) {
    char p[256];
    strncpy(p, fullname.c_str(), sizeof(p) - 1);
    return string{basename(p)};
}

inline bool IsDirReadable(const std::string& dirname) { return (access(dirname.c_str(), R_OK) != -1); }

inline bool IsDirWritable(const std::string& dirname) { return (access(dirname.c_str(), W_OK) != -1); }

inline bool IsFile(const std::string& pathname) {
    struct stat st;
    if (lstat(pathname.c_str(), &st) == -1) return false;
    if (S_ISDIR(st.st_mode))
        return false;
    else
        return true;
}

inline time_t GetLastModificationTime(const std::string& pathname) {
    struct stat st;
    if (lstat(pathname.c_str(), &st) == -1) return 0;
    return st.st_mtime;
}

inline off_t GetFileSize(const std::string& pathname) {
    struct stat st;
    if (lstat(pathname.c_str(), &st) == -1) return -1;
    return st.st_size;
}

inline bool IsDir(const std::string& pathname) {
    struct stat st;
    if (lstat(pathname.c_str(), &st) == -1) return false;
    if (S_ISDIR(st.st_mode))
        return true;
    else
        return false;
}

inline string GetLastLineOfFile(const std::string& pathname) {
    std::ifstream read(pathname, std::ios_base::ate);  // open file
    std::string tmp;

    if (read) {
        long length = read.tellg();  // get file size

        // loop backward over the file
        for (long i = length - 2; i > 0; i--) {
            read.seekg(i);
            char c = read.get();
            if (c == '\r' || c == '\n') break;
        }

        std::getline(read, tmp);  // read last line
    }
    return tmp;
}

enum ZergFileType { QUOTE_DUMP, CUSTOM_DATA };

struct ZergFileHeader {
    int16_t magic_num;
    int16_t version;
    int16_t create_timezone;
    uint32_t create_date;
    uint32_t create_time;
    char create_host[16];
    char create_domain[16];
    char create_user[16];
    char create_type[16];
    char checksum[64];
    char data_type[64];

    ZergFileHeader& FileType(ZergFileType t) {
        magic_num = t;
        return *this;
    }
    ZergFileHeader& DataType(const char* data_type_) {
        strncpy(data_type, data_type_, 63);
        return *this;
    }
    ZergFileHeader& Public() {
        strcpy(create_type, "PUBLIC");
        return *this;
    }
    ZergFileHeader& Private() {
        strcpy(create_type, "PRIVATE");
        return *this;
    }
};

struct ZergFile {
    ZergFileHeader CreateFile() {
        ztool::Clock<> clock;
        clock.Update();
        ZergFileHeader header;
        memset(&header, 0, sizeof(header));
        header.create_date = static_cast<uint32_t>(clock.DateToInt());
        header.create_time = static_cast<uint32_t>(clock.TimeToInt());
        char name[200];
        getlogin_r(name, 199);
        strcpy(header.create_user, name);
        return header;
    }
};

struct SoInfo {
    void* handler;
    int major;
    int minor;
    int patch;
    std::string build;
    std::string compiler;
    std::string compiler_version;
    std::string so_path;
};

inline SoInfo loadSO(const std::string& so_path, char type = RTLD_LAZY) {
    if (!ztool::IsFileExisted(so_path)) {
        THROW_ZERG_EXCEPTION("cannot found so file in path: " << so_path);
    }
    SoInfo info;
    info.so_path = so_path;
    info.handler = dlopen(so_path.c_str(), type);
    if (!info.handler) {
        THROW_ZERG_EXCEPTION("cannot open so file in path: " << so_path << " for the reason:" << dlerror());
    }
    return info;
}

inline bool read_trading_days(const std::string& day_file_path, std::vector<int>& days) {
    days.clear();
    auto path = FileExpandUser(day_file_path);
    std::ifstream ifs(path, std::ifstream::in);

    if (ifs.is_open()) {
        string s;
        while (getline(ifs, s)) {
            ztool::Trim(s);
            if (!s.empty() && std::isdigit(s[0])) {
                days.push_back(std::atoi(s.c_str()));
            }
        }
        ifs.close();
    } else {
        return false;
    }
    std::sort(days.begin(), days.end());
    return true;
}

// like /tmp/*.csv
inline std::unordered_map<std::string, std::string> path_wildcard(const std::string& path) {
    std::unordered_map<std::string, std::string> ret;
    std::string base_name = ztool::Basename(path);
    auto pos = base_name.find('*');
    if (pos != std::string::npos) {
        std::string prefix = base_name.substr(0, pos);
        std::string suffix = base_name.substr(pos + 1);
        std::string dir_path = ztool::Dirname(path) + "/";
        DIR* dir = opendir(dir_path.c_str());
        if (dir) {
            struct dirent* ent;
            while ((ent = readdir(dir)) != nullptr) {
                string file_name = ent->d_name;
                if (ztool::start_with(file_name, prefix) && ztool::end_with(file_name, suffix)) {
                    string matched = file_name.substr(prefix.size(), file_name.size() - prefix.size() - suffix.size());
                    ret.insert({matched, dir_path + file_name});
                }
            }
            closedir(dir);
        }
    }
    return ret;
}

/**
 *
 * @param dir path to traverse
 * @param func the callback function to process each file
 * @param is_enter_sub
 * @param need_folder
 */
inline void dir_traversal(const std::string& dir, void (*func)(const std::string&), bool is_enter_sub = false,
                          bool need_folder = false) {
    std::string full_path;
    if (dir.empty())
        full_path = "";
    else {
        if (dir[dir.size() - 1] == '/')
            full_path = dir;
        else
            full_path = dir + "/";
    }
    DIR* d;
    struct dirent* file;
    struct stat sb {};
    if (!(d = opendir(full_path.c_str()))) return;
    while ((file = readdir(d)) != nullptr) {
        if (file->d_name[0] == '.') continue;
        std::string fname = full_path + file->d_name;
        stat(fname.c_str(), &sb);
        if (S_ISDIR(sb.st_mode)) {
            if (is_enter_sub)
                dir_traversal(fname, func, is_enter_sub);
            else if (need_folder)
                func(std::string("\x01") + fname);
        } else
            func(fname);
    }
    closedir(d);
}

}  // namespace ztool

#endif
