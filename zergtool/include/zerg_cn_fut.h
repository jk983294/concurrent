#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace ztool {
int GetCobFromDir(std::string path);

struct CnFutUkeyMeta {
    std::string pdt;
    std::string exch;
    int ukey;
};

struct CnFutUkeyMetaMap {
    bool read(std::string path = "/opt/version/latest/etc/future_ukey.csv");
    CnFutUkeyMeta* find(const std::string& pdt) { return pdt2meta[pdt]; }
    int get_ukey(const std::string& pdt, int maturity);
    int get_ukey(std::string ins);
    std::string get_pdt(int ukey);
    std::unordered_map<std::string, CnFutUkeyMeta*> pdt2meta;
    std::unordered_map<int, CnFutUkeyMeta*> ukey2meta;
    std::vector<CnFutUkeyMeta> metas;
};
}  // namespace ztool