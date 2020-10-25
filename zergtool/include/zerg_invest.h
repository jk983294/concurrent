#ifndef CONCURRENT_ZERG_INVEST_H
#define CONCURRENT_ZERG_INVEST_H

#include <zerg_string.h>

namespace ztool {
/**
 * cu2006 -> (cu, 2006)
 */
inline std::pair<std::string, int32_t> SplitFuturesMaturity(const std::string& inst_id) {
    std::pair<std::string, int> ret = {"", 0};
    size_t len = inst_id.length();
    if (len < 4)
        return ret;
    else {  // supposed to be xYMM or xYYMM
        std::string tail = inst_id.substr(len - 3, len);
        if (tail.find_first_not_of("1234567890") != std::string::npos) return {"", 0};
        std::string tmp = inst_id.substr(len - 4, 1);
        if (tmp[0] >= '0' && tmp[0] <= '9') {  // xYYMM
            if (len < 5) return {"", 0};
            ret.first = inst_id.substr(0, len - 4);
            ret.second = std::stoi(inst_id.substr(len - 4, len));
            return ret;
        } else {  // xYMM
            ret.first = inst_id.substr(0, len - 3);
            ret.second = std::stoi(inst_id.substr(len - 3, len));
            return ret;
        }
    }
}

inline std::string GetFuturesProduct(const std::string& inst_id) {
    string pdt;
    std::tie(pdt, std::ignore) = SplitFuturesMaturity(inst_id);
    return pdt;
}
}  // namespace ztool

#endif
