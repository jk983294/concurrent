#ifndef ZERG_ADMIN_H
#define ZERG_ADMIN_H

#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "zerg_exception.h"
#include "zerg_time.h"
#include "ztool.h"

using namespace std;

namespace ztool {
struct __attribute__((packed)) AdminShmData {
    char cmd[1024] = {};
    char issuer[128] = {};
    long create_time = 0;  // microseconds since UNIX EPOCH
    long update_time = 0;  // microseconds since UNIX EPOCH
};

struct Admin {
    bool shm_status{false};
    std::string shm_key;
    string shm_name{"admin_"};
    AdminShmData* pAdminShm{nullptr};
    long last_cmd_time{0};

    Admin(const std::string& shm_key_) : shm_key{shm_key_} {
        if (shm_key.empty()) {
            LOG_AND_THROW("admin key cannot be empty");
        }
        shm_name += shm_key;
    }

    Admin& OpenForCreate() {
        auto fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
        fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            THROW_ZERG_EXCEPTION("cannot create shm for " << shm_name);
        }
        fchmod(fd, 0777);
        auto ret = ftruncate(fd, sizeof(AdminShmData));
        if (ret != 0) {
            THROW_ZERG_EXCEPTION("failed to truncate shm " << shm_name);
        }
        char* p_mem = (char*)mmap(nullptr, sizeof(AdminShmData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p_mem == MAP_FAILED) {
            THROW_ZERG_EXCEPTION("failed to mmap shm " << shm_name);
        }
        printf("AdminShm %s created with size:%zu\n", shm_name.c_str(), sizeof(AdminShmData));
        pAdminShm = reinterpret_cast<AdminShmData*>(p_mem);
        memset(p_mem, 0, sizeof(AdminShmData));
        return *this;
    }

    Admin& OpenForRead() {
        auto shm_length = sizeof(AdminShmData);
        auto fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
        if (fd == -1) {
            shm_status = false;
            perror("shm open failed\n");
            close(fd);
            return *this;
        }
        char* p_mem = (char*)mmap(nullptr, shm_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p_mem == MAP_FAILED) {
            shm_status = false;
            perror("MEM MAP FAILED\n");
            return *this;
        }

        pAdminShm = reinterpret_cast<AdminShmData*>(p_mem);
        shm_status = true;
        printf("AdminShm %s opened, size %zu\n", shm_name.c_str(), shm_length);
        return *this;
    }

    void IssueCmd(const string& cmd) const {
        if (!pAdminShm) {
            printf("pAdminShm not ready\n");
            return;
        }
        if (cmd.empty() || cmd.size() > sizeof(pAdminShm->cmd)) {
            printf("cmd length not correct\n");
            return;
        }
        strcpy(pAdminShm->cmd, cmd.c_str());
        uid_t uid = geteuid();
        struct passwd* pw = getpwuid(uid);
        if (pw) {
            strcpy(pAdminShm->issuer, pw->pw_name);
        }
        pAdminShm->update_time = ztool::GetMicrosecondsSinceEpoch();
        printf("issue cmd '%s' from %s at %ld success\n", pAdminShm->cmd, pAdminShm->issuer, pAdminShm->update_time);
    }

    string ReadCmd() {
        if (pAdminShm && pAdminShm->update_time > last_cmd_time) {
            // printf("recv cmd '%s' from %s at %ld\n", pAdminShm->cmd, pAdminShm->issuer, pAdminShm->update_time);
            last_cmd_time = pAdminShm->update_time;
            return string{pAdminShm->cmd};
        } else {
            return "";
        }
    }
};
}  // namespace ztool

#endif
