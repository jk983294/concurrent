#ifndef CONCURRENT_ZERG_TEMPLATE_H
#define CONCURRENT_ZERG_TEMPLATE_H

#include <cstdint>
#include <type_traits>

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
}  // namespace ztool

#endif
