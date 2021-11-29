#ifndef CONCURRENT_ZERG_INVEST_H
#define CONCURRENT_ZERG_INVEST_H

#include <zerg_string.h>
#include <zerg_time.h>

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

inline int32_t GetToday(std::vector<int32_t>& dl, int32_t offset = 0) {
    int day = static_cast<int>(ztool::now_cob());
    auto itr = std::find(dl.begin(), dl.end(), day);
    if (itr + offset < dl.end())
        return *(itr + offset);
    else
        return -1;
}
}  // namespace ztool

#endif
