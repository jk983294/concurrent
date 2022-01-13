#ifndef CONCURRENT_ZERG_INVEST_H
#define CONCURRENT_ZERG_INVEST_H

#include <zerg_string.h>
#include <zerg_time.h>

inline constexpr int ctp_total_minute(int h, int m) {
    int minute = h * 60 + m;
    if (h < 18) minute += 24 * 60;
    return minute;
}

constexpr int ctp_night_start_minute = ctp_total_minute(21, 0);
constexpr int ctp_night_end_minute = ctp_total_minute(2, 30);
constexpr int ctp_day_start_minute = ctp_total_minute(9, 0);
constexpr int ctp_day_end_minute = ctp_total_minute(15, 0);

inline bool IsInCtpTradingHour(int total_minute, bool is_night_session) {
    if (is_night_session)
        return (total_minute >= ctp_night_start_minute && total_minute <= ctp_night_end_minute);
    else
        return (total_minute >= ctp_day_start_minute && total_minute <= ctp_day_end_minute);
}

inline bool IsInCtpTradingHourAll(int total_minute) {
    return (total_minute >= ctp_night_start_minute && total_minute <= ctp_night_end_minute) ||
           (total_minute >= ctp_day_start_minute && total_minute <= ctp_day_end_minute);
}

/**
 * 21:00 - 24:00
 */
inline bool is_in_early_night_session(int time) { return time > ztool::day_night_split; }
/**
 * 00:00 - 3:00
 */
inline bool is_in_late_night_session(int time) { return time < 30000000; }
/**
 * 21:00 - 3:00
 */
inline bool is_in_night_session(int time) { return (time < 30000000 || time > ztool::day_night_split); }
/**
 * 9:00 - 15:15
 */
inline bool is_in_day_session(int time) { return (time > ztool::day_start_split && time < ztool::day_night_split); }

struct CtpInstrumentDaily {
    std::string ContractCode;
    int TradingDay{-1};
    int LastTradingDate{-1};
    int Volume{-1};
    int Multiplier{-1};
    double PrevSettlePrice{NAN};
    double OpenPrice{NAN};
    double HighPrice{NAN};
    double LowPrice{NAN};
    double LittlestChangeUnit{NAN};
    double PriceLimit{NAN};
    double MarginRatio{NAN};
    // from tmr profile
    double ClosePrice{NAN};    // get from md, adjust using tmr profile
    double OpenInterest{NAN};  // get from md, adjust using tmr profile
    double SettlePrice{NAN};   // only from tmr profile
};

std::ostream& operator<<(std::ostream& s, const CtpInstrumentDaily& data);
std::vector<CtpInstrumentDaily> ctp_daily_from_csv(const std::string& path);
bool ctp_daily_to_csv(const std::string& filename, std::vector<CtpInstrumentDaily>& daily_datum);

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
