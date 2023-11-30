#include <csv.h>
#include <zerg_file.h>
#include <zerg_invest.h>
#include <zerg_math.h>

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

void CtpInstrumentProfile::set_nan() {
    if (PriceTick == std::numeric_limits<double>::max()) PriceTick = NAN;
    if (LongMarginRatio == std::numeric_limits<double>::max()) LongMarginRatio = NAN;
    if (ShortMarginRatio == std::numeric_limits<double>::max()) ShortMarginRatio = NAN;
    if (PreSettlementPrice == std::numeric_limits<double>::max()) PreSettlementPrice = NAN;
    if (PreClosePrice == std::numeric_limits<double>::max()) PreClosePrice = NAN;
    if (PreOpenInterest == std::numeric_limits<double>::max()) PreOpenInterest = NAN;
    if (UpperLimitPrice == std::numeric_limits<double>::max()) UpperLimitPrice = NAN;
    if (LowerLimitPrice == std::numeric_limits<double>::max()) LowerLimitPrice = NAN;
}

void set_from(CtpInstrumentDaily& daily, const CtpInstrumentProfile& profile) {
    daily.ContractCode = profile.InstrumentID + "." + profile.ExchangeID;
    daily.PrevSettlePrice = profile.PreSettlementPrice;
    daily.LittlestChangeUnit = profile.PriceTick;

    double limit_up = NAN, limit_down = NAN;
    if (!IsNanField(daily.PrevSettlePrice) && !ztool::FloatEqual(daily.PrevSettlePrice, 0.)) {
        if (!IsNanField(profile.UpperLimitPrice) && !ztool::FloatEqual(profile.UpperLimitPrice, 0.))
            limit_up = ztool::round_up(profile.UpperLimitPrice / daily.PrevSettlePrice - 1.0, 3);
        if (!IsNanField(profile.LowerLimitPrice) && !ztool::FloatEqual(profile.LowerLimitPrice, 0.))
            limit_down = ztool::round_up(1.0 - profile.LowerLimitPrice / daily.PrevSettlePrice, 3);
    }
    if (std::isfinite(limit_up) && std::isfinite(limit_down)) {
        daily.PriceLimit = std::min(limit_down, limit_up);
    } else if (!std::isfinite(limit_up)) {
        daily.PriceLimit = limit_down;
    } else if (!std::isfinite(limit_down)) {
        daily.PriceLimit = limit_up;
    }

    if (std::isfinite(profile.LongMarginRatio) && std::isfinite(profile.ShortMarginRatio)) {
        daily.MarginRatio = std::max(profile.LongMarginRatio, profile.ShortMarginRatio);
    } else if (!std::isfinite(profile.LongMarginRatio)) {
        daily.MarginRatio = profile.ShortMarginRatio;
    } else if (!std::isfinite(profile.ShortMarginRatio)) {
        daily.MarginRatio = profile.LongMarginRatio;
    }
    daily.Multiplier = profile.VolumeMultiple;
    daily.OpenInterest = profile.PreOpenInterest;
}

std::ostream& operator<<(std::ostream& s, const CtpInstrumentProfile& data) {
    s << data.InstrumentID << "," << data.ExchangeID << "," << data.InstrumentName << "," << data.ProductID << ","
      << data.ProductClass << "," << data.DeliveryYear << "," << data.DeliveryMonth << "," << data.MaxMarketOrderVolume
      << "," << data.MinMarketOrderVolume << "," << data.MaxLimitOrderVolume << "," << data.MinLimitOrderVolume << ","
      << data.VolumeMultiple << "," << data.PriceTick << "," << data.CreateDate << "," << data.OpenDate << ","
      << data.ExpireDate << "," << data.StartDelivDate << "," << data.EndDelivDate << "," << data.InstLifePhase << ","
      << data.IsTrading << "," << data.PositionType << "," << data.PositionDateType << "," << data.LongMarginRatio
      << "," << data.ShortMarginRatio << "," << data.PreSettlementPrice << "," << data.PreClosePrice << ","
      << data.PreOpenInterest << "," << data.UpperLimitPrice << "," << data.LowerLimitPrice;
    return s;
}

std::vector<CtpInstrumentProfile> ctp_profile_from_csv(const std::string& path) {
    string file_path = path;
    if (!ztool::end_with(path, ".day") && ztool::IsFileExisted(file_path + ".day")) {
        file_path = file_path + ".day";
    }
    std::vector<CtpInstrumentProfile> ret;
    io::CSVReader<29> infile(file_path);
    infile.read_header(io::ignore_extra_column, "合约代码", "交易所代码", "合约名称", "产品代码", "产品类型",
                       "交割年份", "交割月", "市价单最大下单量", "市价单最小下单量", "限价单最大下单量",
                       "限价单最小下单量", "合约数量乘数", "最小变动价位", "创建日", "上市日", "到期日", "开始交割日",
                       "结束交割日", "合约生命周期状态", "当前是否交易", "持仓类型", "持仓日期类型", "多头保证金率",
                       "空头保证金率", "昨日结算价", "昨日收盘价", "昨日持仓量", "涨停价", "跌停价");

    CtpInstrumentProfile data;
    while (infile.read_row(data.InstrumentID, data.ExchangeID, data.InstrumentName, data.ProductID, data.ProductClass,
                           data.DeliveryYear, data.DeliveryMonth, data.MaxMarketOrderVolume, data.MinMarketOrderVolume,
                           data.MaxLimitOrderVolume, data.MinLimitOrderVolume, data.VolumeMultiple, data.PriceTick,
                           data.CreateDate, data.OpenDate, data.ExpireDate, data.StartDelivDate, data.EndDelivDate,
                           data.InstLifePhase, data.IsTrading, data.PositionType, data.PositionDateType,
                           data.LongMarginRatio, data.ShortMarginRatio, data.PreSettlementPrice, data.PreClosePrice,
                           data.PreOpenInterest, data.UpperLimitPrice, data.LowerLimitPrice)) {
        ret.push_back(data);
    }
    return ret;
}

bool ctp_profile_to_csv(const std::string& filename, std::vector<CtpInstrumentProfile>& profiles) {
    std::ofstream ofs(filename, std::ofstream::out | std::ofstream::trunc);

    if (!ofs) {
        return false;
    }

    ofs << "合约代码,交易所代码,合约名称,产品代码,产品类型,交割年份,交割月,市价单最大下单量,市价单最小下单量,"
           "限价单最大下单量,限价单最小下单量,合约数量乘数,最小变动价位,创建日,上市日,到期日,开始交割日,结束交割日,"
           "合约生命周期状态,当前是否交易,持仓类型,持仓日期类型,多头保证金率,空头保证金率,昨日结算价,昨日收盘价,"
           "昨日持仓量,涨停价,跌停价"
        << std::endl;

    std::sort(profiles.begin(), profiles.end(), [](const CtpInstrumentProfile& lhs, const CtpInstrumentProfile& rhs) {
        return lhs.InstrumentID < rhs.InstrumentID;
    });
    for (const auto& field : profiles) {
        ofs << field << std::endl;
    }
    ofs.close();
    return true;
}