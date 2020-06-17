#ifndef ZERG_STRING_SPLIT_H
#define ZERG_STRING_SPLIT_H

#include <bitset>
#include <cstring>
#include <string>
#include <vector>

namespace ztool {
class SmartChar {
    char* pointer;
    std::size_t* refs;

    void clear() {
        if (!--*refs) {
            free(pointer);
            delete refs;
        }
    }

public:
    SmartChar(char* p = nullptr) : pointer(p), refs(new std::size_t(1)) {}

    SmartChar(const SmartChar& other) : pointer(other.pointer), refs(other.refs) { ++*refs; }
    ~SmartChar() { clear(); }

    SmartChar& operator=(const SmartChar& other) {
        if (this != &other) {
            clear();
            pointer = other.pointer;
            refs = other.refs;
            ++*refs;
        }
        return *this;
    }

    SmartChar& operator=(char* p) {
        if (pointer != p) {
            pointer = p;
            *refs = 1;
        }
        return *this;
    }

    char& operator*() { return *pointer; }

    const char& operator*() const { return *pointer; }

    char* operator->() { return pointer; }

    const char* operator->() const { return pointer; }

    std::size_t getCounts() { return *refs; }
};

/* split string function */
struct SplitResult {
    size_t len_;
    size_t size_;
    std::vector<std::pair<const char*, size_t> > pos_offset;
    size_t size() { return size_; }
    SmartChar kept_str;

    SplitResult() { kept_str = nullptr; };

    SplitResult(const SplitResult& lhs) {
        len_ = lhs.len_;
        size_ = lhs.size_;
        kept_str = lhs.kept_str;
        pos_offset = lhs.pos_offset;
    }

    SplitResult(SplitResult&& lhs) {
        len_ = lhs.len_;
        size_ = lhs.size_;
        pos_offset = lhs.pos_offset;
        kept_str = lhs.kept_str;
    }

    SplitResult& operator=(const SplitResult& other) {
        if (&other == this) return *this;
        len_ = other.len_;
        size_ = other.size_;
        kept_str = other.kept_str;
        pos_offset = other.pos_offset;
        return *this;
    }

    ~SplitResult() = default;

    const char* get(size_t n) {
        if (n >= size_) {
            return "";
        }
        char* plit = const_cast<char*>(pos_offset[n].first) + pos_offset[n].second;
        *plit = '\0';
        return pos_offset[n].first;
    }

    const char* getFast(size_t n) {
        char* p_ret = (char*)malloc((pos_offset[n].second + 1) * sizeof(char));
        if (n >= size_) {
            return "";
        } else
            strncpy(p_ret, const_cast<char*>(pos_offset[n].first), pos_offset[n].second);
        p_ret[pos_offset[n].second] = '\0';
        return p_ret;
    }
};

namespace DelimiterPolicy {
class MergeDoubleDelimiter {
public:
    static SplitResult Split(const char* str, const char* delimiter) {
        SplitResult ret;
        ret.size_ = 0;
        std::vector<std::pair<const char*, size_t> > output;
        std::bitset<255> delims;  // bit set stores delimiters, make it compact
        while (*delimiter) {
            auto code = static_cast<unsigned char>(*delimiter++);
            delims[code] = true;
        }
        const char* beg = str;
        // bool in_token = false;
        size_t len = 0;  // measuring token length
        bool continuity = false;
        for (auto it = str; *it != '\0'; ++it) {
            ++len;
            if (delims[(uint8_t)*it]) {
                if (continuity) {
                    beg = it + 1;
                    --len;
                    continue;
                }
                continuity = true;
                output.push_back(std::make_pair(beg, len - 1));
                beg = it + 1;
                len = 0;
                ret.size_++;
            } else {
                continuity = false;
            }
        }
        if (*beg != '\0') {
            output.push_back(std::make_pair(beg, len));
            ret.size_++;
        }
        ret.pos_offset.swap(output);
        return ret;
    }
};

class KeepDoubleDelimiter {
public:
    static SplitResult Split(const char* str, const char* delimiter) {
        SplitResult ret;
        ret.size_ = 0;
        std::vector<std::pair<const char*, size_t> > output;
        std::bitset<255> delims;  // bit set stores delimiters, make it compact
        while (*delimiter) {
            auto code = static_cast<unsigned char>(*delimiter++);
            delims[code] = true;
        }
        const char* beg = str;
        // bool in_token = false;
        size_t len = 0;  // measuring token length
        for (auto it = str; *it != '\0'; ++it) {
            ++len;
            if (delims[(uint8_t)*it]) {
                output.push_back(std::make_pair(beg, len - 1));
                beg = it + 1;
                len = 0;
                ret.size_++;
            }
        }
        if (*beg != '\0') {
            output.push_back(std::make_pair(beg, len));
            ret.size_++;
        }
        ret.pos_offset.swap(output);
        return ret;
    }
};
}  // namespace DelimiterPolicy

template <typename policy = DelimiterPolicy::MergeDoubleDelimiter>
inline SplitResult SplitStringFast(const char* str, const char* delimiter) {
    return policy::Split(str, delimiter);
}

template <typename policy = DelimiterPolicy::MergeDoubleDelimiter>
inline SplitResult SplitStringFast(const std::string& str, const char* delimiter) {
    return SplitStringFast<policy>(str.c_str(), delimiter);
}

template <typename policy = DelimiterPolicy::MergeDoubleDelimiter>
inline SplitResult SplitString(const char* str, const char* delimiter) {
    auto len = strlen(str);
    char* kept_str = (char*)malloc(sizeof(char) * (len + 1));
    strcpy(kept_str, str);
    auto ret = SplitStringFast<policy>(kept_str, delimiter);
    ret.kept_str = kept_str;
    ret.len_ = len;
    return ret;
}

template <typename policy = DelimiterPolicy::MergeDoubleDelimiter>
inline SplitResult SplitString(const std::string& str, const char* delimiter) {
    auto len = str.size();
    char* kept_str = (char*)malloc(sizeof(char) * (len + 1));
    strcpy(kept_str, str.c_str());
    auto ret = SplitStringFast<policy>(kept_str, delimiter);
    ret.kept_str = kept_str;
    ret.len_ = len;
    return ret;
}
}  // namespace ztool

#endif
