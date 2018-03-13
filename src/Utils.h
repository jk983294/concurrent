#ifndef CONCURRENT_UTILS_H
#define CONCURRENT_UTILS_H

#include <iostream>
#include <string>
#include <vector>

namespace frenzy {

std::string time_string();

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[ ";
    for (const auto& i : v) os << i << " ";
    return os << "]";
}
}

#endif
