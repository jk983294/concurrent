#ifndef CONCURRENT_ZERG_TEMPLATE_H
#define CONCURRENT_ZERG_TEMPLATE_H

#include <type_traits>

namespace ztool {
template <typename Enumeration>
auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}
}  // namespace ztool

#endif
