#ifndef _ZERG_STRING_H_
#define _ZERG_STRING_H_

#include <iconv.h>
#include <algorithm>
#include <bitset>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <locale>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "string_split.h"

using std::string;

namespace ztool {
inline std::vector<std::string> split(const std::string& str, char delimiter = ' ');
inline std::vector<std::string> splits(const std::string& s, const char* separator = " |,");
inline std::pair<std::string, std::string> SplitInstrumentID(const std::string& str);
inline std::string gbk2utf(const char* source);
inline std::string gbk2utf(const std::string& str);
inline std::string ToUpperCaseCopy(const std::string& s);
inline std::string ToLowerCaseCopy(const std::string& s);
inline std::string ToUpperCase(std::string& str);
inline std::string ToLowerCase(std::string& str);
inline void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
inline std::string ReplaceAllCopy(const std::string& str, const std::string& from, const std::string& to);
inline std::string& LTrim(std::string& s);
inline std::string& RTrim(std::string& s);
inline std::string& Trim(std::string& s);
inline std::string LTrimCopy(const std::string& s);
inline std::string RTrimCopy(const std::string& s);
inline std::string TrimCopy(const std::string& s);
inline void text_word_warp(const std::string& text, size_t maxLineSize, std::vector<std::string>& lines);
inline std::string get_bool_string(bool value);
inline std::string get_bool_string(int value);
inline bool start_with(const std::string& s, const std::string& p);
inline bool end_with(const std::string& s, const std::string& p);
inline std::string GenerateRandomString(size_t length, uint32_t seed = 0);
inline std::vector<std::string> expand_names(const std::string& str);
inline std::string to_string_high_precision(double value);

inline int code_convert(const char* from_charset, const char* to_charset, char* inBuf, size_t inLen, char* outBuf,
                        size_t outLen) {
    iconv_t cd;
    char** pin = &inBuf;
    char** pout = &outBuf;

    cd = iconv_open(to_charset, from_charset);
    if (cd == nullptr) return -1;
    memset(outBuf, 0, outLen);
    if ((int)iconv(cd, pin, &inLen, pout, &outLen) == -1) return -1;
    iconv_close(cd);
    **pout = '\0';
    return 0;
}

inline std::string gbk2utf(const char* source) {
    char origin[128], converted[128];
    strcpy(origin, source);
    code_convert((char*)"gbk", (char*)"utf-8", origin, strlen(source), converted, 128);
    return std::string(converted);
}

inline std::string gbk2utf(const std::string& str) { return gbk2utf(str.c_str()); }

inline std::string ToUpperCaseCopy(const std::string& s) {
    std::string str = s;
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

inline std::string ToLowerCaseCopy(const std::string& s) {
    std::string str = s;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

inline std::string ToUpperCase(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

inline std::string ToLowerCase(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

inline void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty() || str.empty()) return;
    auto start_pos = str.find(from);
    while (start_pos != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos = str.find(from, start_pos + to.length());  //+to.length in case 'to' contains 'from'
    }
}

inline std::string ReplaceAllCopy(const std::string& str, const std::string& from, const std::string& to) {
    std::string ret = str;
    ReplaceAll(ret, from, to);
    return ret;
}

// trim from start
inline std::string& LTrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
inline std::string& RTrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
inline std::string& Trim(std::string& s) { return LTrim(RTrim(s)); }

// trim from start
inline std::string LTrimCopy(const std::string& s) {
    auto str = s;
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return str;
}

// trim from end
inline std::string RTrimCopy(const std::string& s) {
    auto str = s;
    str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
              str.end());
    return str;
}

// trim from both ends
inline std::string TrimCopy(const std::string& s) {
    auto str = s;
    str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
              str.end());
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return str;
}

inline void text_word_warp(const std::string& text, size_t maxLineSize, std::vector<std::string>& lines) {
    maxLineSize = std::max(maxLineSize, (size_t)16);
    size_t remaining = text.size();
    size_t textSize = text.size();
    size_t start{0};
    const char* pData = text.c_str();

    while (remaining > maxLineSize) {
        size_t end = start + maxLineSize;
        // find last whitespace
        while (!isspace(pData[end - 1] && end > start)) end--;
        // find last non whitespace
        while (isspace(pData[end - 1] && end > start)) end--;

        bool truncate = false;
        if (end == start) {
            end = start + maxLineSize - 1;
            truncate = true;
        }

        std::string line(text.substr(start, end - start));
        if (truncate) line += '-';

        lines.push_back(line);
        start = end;

        while (isspace(pData[start]) && start < textSize) ++start;
        remaining = textSize - start;
    }

    if (remaining > 0) {
        lines.push_back(text.substr(start, remaining));
    }
}

template <class T>
inline std::string string_join(const std::vector<T>& v, char delimiter = ' ') {
    std::ostringstream os;
    if (v.size() > 0) {
        os << v[0];
        for (size_t i = 1; i < v.size(); ++i) {
            os << delimiter << v[i];
        }
    }
    return os.str();
}

inline std::string get_bool_string(int value) {
    if (value)
        return "true";
    else
        return "false";
}

inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    result.push_back(str.substr(start, end));
    return result;
}

inline std::vector<std::string> splits(const std::string& s, const char* separator) {
    std::vector<std::string> ret;
    auto split_result = ztool::SplitString(s, separator);
    for (uint i = 0; i < split_result.size(); ++i) {
        ret.push_back(ztool::TrimCopy(split_result.get(i)));
    }
    return ret;
}

inline std::pair<std::string, std::string> SplitInstrumentID(const std::string& str) {
    std::pair<std::string, std::string> ret;
    auto idx = str.find('.');
    if (idx == std::string::npos) {
        ret.first = str;
    } else {
        ret.first = str.substr(0, idx);
        ret.second = str.substr(idx + 1);
    }
    return ret;
}

inline bool start_with(const std::string& s, const std::string& p) { return s.find(p) == 0; }
inline bool end_with(const std::string& s, const std::string& p) {
    if (s.length() >= p.length()) {
        return (0 == s.compare(s.length() - p.length(), p.length(), p));
    } else {
        return false;
    }
}

inline std::string GenerateRandomString(size_t length, uint32_t seed) {
    if (!seed) {  // if seed is zero, will init seed
        std::random_device rd;
        seed = rd();
    }
    std::mt19937 generator(seed);

    char* s = (char*)malloc((length + 1) * sizeof(char));
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::uniform_int_distribution<int> uid(0, sizeof(alphanum) - 2);
    for (size_t i = 0; i < length; ++i) {
        s[i] = alphanum[uid(generator)];
    }

    s[length] = '\0';
    auto ret = std::string(s);
    free(s);
    return ret;
}

inline std::vector<std::string> expand_names(const std::string& str) {
    std::vector<std::string> ret;
    auto name_patterns = ztool::split(str, ',');
    int tmp_num = 0;
    for (auto& pattern : name_patterns) {
        if (pattern.empty()) continue;
        auto p1 = pattern.find('[');
        if (p1 == std::string::npos) {
            ret.push_back(pattern);
            tmp_num++;
        } else {
            auto p2 = pattern.find(']');
            if (p2 == string::npos || p2 < p1) {
                ret.push_back(pattern);
                tmp_num++;
            } else {
                string prefix = pattern.substr(0, p1);
                string suffix = pattern.substr(p2 + 1);
                string wildcards = pattern.substr(p1 + 1, p2 - p1 - 1);
                if (wildcards.find('-') != string::npos) {
                    auto lets = ztool::split(wildcards, '-');
                    if (lets.size() == 2) {
                        int start_idx = std::stoi(lets[0]);
                        int end_idx = std::stoi(lets[1]);
                        for (int i = start_idx; i <= end_idx; ++i) {
                            ret.push_back(prefix + std::to_string(i) + suffix);
                            tmp_num++;
                        }
                    } else {
                        ret.push_back(pattern);
                        tmp_num++;
                    }
                } else {
                    ret.push_back(pattern);
                    tmp_num++;
                }
            }
        }
    }
    return ret;
}

inline std::string to_string_high_precision(double value) {
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%g", value);
    return std::string(buffer);
}
}  // namespace ztool

#endif
