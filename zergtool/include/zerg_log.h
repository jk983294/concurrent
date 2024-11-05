#ifndef CONCURRENT_ZERG_LOG_H
#define CONCURRENT_ZERG_LOG_H

#include <zerg_time.h>
#include <unistd.h>

#define ZLOG(...)                                       \
    do {                                                \
        char msg[256];                                  \
        sprintf(msg, __VA_ARGS__);                      \
        std::string now_str = ztool::now_string();      \
        printf("[%s]%s\n", now_str.c_str(), msg);       \
        fflush(stdout);                                 \
    } while (0)

#define ZLOG_THROW(...)                                 \
    do {                                                \
        char error[256];                                \
        sprintf(error, __VA_ARGS__);                    \
        ZLOG("%s", error);                              \
        usleep(1000);                                   \
        throw std::runtime_error(error);                \
    } while (0)

#endif