#ifndef CONCURRENT_UTILS_H
#define CONCURRENT_UTILS_H

#include <iostream>
#include <string>
#include <vector>

namespace frenzy {

std::string timespec2string(const timespec& ts);
std::string time_string();
size_t nextPowerOf2(size_t n);

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[ ";
    for (const auto& i : v) os << i << " ";
    return os << "]";
}
}

#endif
