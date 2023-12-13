#include <csv.h>
#include <zerg_cn_fut.h>
#include <zerg_file.h>
#include <zerg_invest.h>

namespace ztool {
int GetCobFromDir(std::string path) {
    if (path.empty()) return -1;
    path = ztool::GetAbsolutePath(path);
    if (ztool::IsDir(path)) {
        path = ztool::ReplaceAllCopy(path, "/", "");
    } else {
        return -1;
    }
    if (path.size() < 8) return -1;
    string cob_str_ = path.substr(path.size() - 8);
    bool all_number = all_of(cob_str_.begin(), cob_str_.end(), [](const char& c) { return c >= '0' && c <= '9'; });
    if (!all_number)
        return -1;
    else
        return std::stoi(cob_str_);
}

std::string CnFutUkeyMetaMap::get_pdt(int ukey) {
    if (ukey > 20000000) {
        ukey = ukey - (ukey % 10000);
    } else if (ukey < 10000) {
        ukey = ukey * 10000;
    }
    auto itr = ukey2meta.find(ukey);
    if (itr == ukey2meta.end()) return "NA";
    else {
        CnFutUkeyMeta* meta = itr->second;
        return meta->pdt + "." + meta->exch;
    }
}

std::string CnFutUkeyMetaMap::get_ins(int ukey) {
    int mat = ukey % 10000;
    int pdt_ukey = ukey - mat;
    auto itr = ukey2meta.find(pdt_ukey);
    if (itr == ukey2meta.end()) return "NA";
    else {
        CnFutUkeyMeta* meta = itr->second;
        return meta->pdt + std::to_string(mat) + "." + meta->exch;
    }
}

int CnFutUkeyMetaMap::get_ukey(std::string ins) {
    auto itr = ins.find_first_of('.');
    if (itr != string::npos) ins = ins.substr(0, itr); // chop off .CFFEX suffix
    std::string pdt;
    int maturity;
    std::tie(pdt, maturity) = SplitFuturesMaturity(ins);
    return get_ukey(pdt, maturity);
}

int CnFutUkeyMetaMap::get_ukey(const std::string& pdt, int maturity) {
    auto* meta = find(pdt);
    if (meta == nullptr) return -1;
    return meta->ukey + maturity;
}

bool CnFutUkeyMetaMap::read(std::string path) {
    path = ztool::FileExpandUser(path);
    metas.clear();
    io::CSVReader<3> infile(path);

    infile.read_header(io::ignore_extra_column, "pdt", "exch", "ukey");

    CnFutUkeyMeta meta;
    while (infile.read_row(meta.pdt, meta.exch, meta.ukey)) {
        metas.push_back(meta);
    }

    pdt2meta.clear();
    ukey2meta.clear();
    for (auto& m : metas) {
        if (pdt2meta.find(m.pdt) != pdt2meta.end()) {
            printf("CnFutUkeyMetaMap::read error, dupe pdt %s\n", m.pdt.c_str());
            return false;
        } else {
            pdt2meta[m.pdt] = &m;
        }

        if (ukey2meta.find(m.ukey) != ukey2meta.end()) {
            printf("CnFutUkeyMetaMap::read error, dupe ukey %d\n", m.ukey);
            return false;
        } else {
            ukey2meta[m.ukey] = &m;
        }
    }
    return true;
}
}