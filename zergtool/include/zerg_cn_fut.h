#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace ztool {
int GetCobFromDir(std::string path);

struct CnFutUkeyMeta {
    std::string pdt;
    std::string exch;
    int ukey{-1};
    int night_end{-1};
    int day_end{-1};
};

struct CnFutUkeyMetaMap {
    bool read(std::string path = "/opt/version/latest/etc/future_ukey.csv");
    CnFutUkeyMeta* find(const std::string& pdt) const;
    CnFutUkeyMeta* find(int pdt_ukey) const;
    int get_ukey(const std::string& pdt, int maturity);
    int get_ukey(std::string ins);
    std::string get_pdt(int ukey);
    std::string get_ins(int ukey);
    std::unordered_map<std::string, CnFutUkeyMeta*> pdt2meta;
    std::unordered_map<int, CnFutUkeyMeta*> ukey2meta;
    std::vector<CnFutUkeyMeta> metas;
};
}  // namespace ztool