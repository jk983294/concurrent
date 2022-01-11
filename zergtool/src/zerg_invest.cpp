#include <csv.h>
#include <zerg_invest.h>

std::ostream& operator<<(std::ostream& s, const CtpInstrumentDaily& data) {
    s << data.ContractCode << "," << data.TradingDay << "," << data.PrevSettlePrice << "," << data.OpenPrice << ","
      << data.HighPrice << "," << data.LowPrice << "," << data.ClosePrice << "," << data.SettlePrice << ","
      << data.Volume << "," << data.OpenInterest << "," << data.LastTradingDate << "," << data.Multiplier << ","
      << data.LittlestChangeUnit << "," << data.PriceLimit << "," << data.MarginRatio;
    return s;
}

std::vector<CtpInstrumentDaily> ctp_daily_from_csv(const string& path) {
    std::vector<CtpInstrumentDaily> ret;
    io::CSVReader<15> infile(path);
    infile.read_header(io::ignore_extra_column, "ContractCode", "TradingDay", "PrevSettlePrice", "OpenPrice",
                       "HighPrice", "LowPrice", "ClosePrice", "SettlePrice", "Volume", "OpenInterest",
                       "LastTradingDate", "Multiplier", "LittlestChangeUnit", "PriceLimit", "MarginRatio");

    CtpInstrumentDaily data;
    while (infile.read_row(data.ContractCode, data.TradingDay, data.PrevSettlePrice, data.OpenPrice, data.HighPrice,
                           data.LowPrice, data.ClosePrice, data.SettlePrice, data.Volume, data.OpenInterest,
                           data.LastTradingDate, data.Multiplier, data.LittlestChangeUnit, data.PriceLimit,
                           data.MarginRatio)) {
        ret.push_back(data);
    }
    return ret;
}

bool ctp_daily_to_csv(const std::string& filename, std::vector<CtpInstrumentDaily>& daily_datum) {
    std::ofstream ofs(filename, std::ofstream::out | std::ofstream::trunc);

    if (!ofs) {
        return false;
    }

    ofs << "ContractCode,TradingDay,PrevSettlePrice,OpenPrice,HighPrice,LowPrice,ClosePrice,SettlePrice,Volume,"
           "OpenInterest,LastTradingDate,Multiplier,LittlestChangeUnit,PriceLimit,MarginRatio"
        << std::endl;

    std::sort(daily_datum.begin(), daily_datum.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.ContractCode < rhs.ContractCode; });
    for (const auto& field : daily_datum) {
        ofs << field << std::endl;
    }
    ofs.close();
    return true;
}