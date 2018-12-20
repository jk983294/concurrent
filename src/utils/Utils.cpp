#include "Utils.h"

namespace frenzy {

std::string timespec2string(const timespec& ts) {
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    char buffer[24];
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u %02u:%02u:%02u.%03u", tm.tm_year + 1900, tm.tm_mon + 1,
                  tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<uint16_t>(ts.tv_nsec / 1000000));
    return std::string(buffer);
}

std::string time_string() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return timespec2string(ts);
}

std::string time_t2string(time_t t1) {
    struct tm tm {};
    localtime_r(&t1, &tm);
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%4u%02u%02u.%02u%02u%02u", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buffer);
}

std::string now_string() {
    time_t tNow = time(nullptr);
    return time_t2string(tNow);
}

size_t nextPowerOf2(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}
}  // namespace frenzy
