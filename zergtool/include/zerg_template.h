#ifndef CONCURRENT_ZERG_TEMPLATE_H
#define CONCURRENT_ZERG_TEMPLATE_H

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

using std::string;

namespace ztool {
template <typename Enumeration>
auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template <typename T>
inline void GetParamFromStr(const string& str, T& out) {
    out = T();
}
template <>
inline void GetParamFromStr<int32_t>(const string& str, int32_t& out) {
    out = std::stoi(str);
}
template <>
inline void GetParamFromStr<float>(const string& str, float& out) {
    out = std::stof(str);
}
template <>
inline void GetParamFromStr<double>(const string& str, double& out) {
    out = std::stod(str);
}
template <>
inline void GetParamFromStr<bool>(const string& str, bool& out) {
    out = (str == "true" || str == "1" || str == "True");
}

template <typename T>
inline T GetParamFromStrByType(const string& str) {
    T out;
    GetParamFromStr<T>(str, out);
    return out;
}
inline int32_t GetInt32FromStr(const string& str) { return GetParamFromStrByType<int32_t>(str); }
inline float GetFloatFromStr(const string& str) { return GetParamFromStrByType<float>(str); }
inline double GetDoubleFromStr(const string& str) { return GetParamFromStrByType<double>(str); }
inline bool GetBoolFromStr(const string& str) { return GetParamFromStrByType<bool>(str); }

template <typename T>
inline string to_string(const T& v) {
    return std::to_string(v);
}

template <>
inline string to_string(const std::string& v) {
    return v;
}

template <typename TContainer>
std::string head(const TContainer& container, size_t n) {
    std::string ret;
    if (n == 0)
        n = container.size();
    else
        n = std::min(n, container.size());
    for (size_t i = 0; i < n; ++i) {
        ret.append(to_string(container[i])).append(",");
    }
    return ret;
}

template <typename T>
bool is_identical(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

template <typename T>
bool is_subset(std::vector<T> a, std::vector<T> sub_a) {
    std::sort(a.begin(), a.end());
    std::sort(sub_a.begin(), sub_a.end());
    return std::includes(a.begin(), a.end(), sub_a.begin(), sub_a.end());
}

}  // namespace ztool

#endif
