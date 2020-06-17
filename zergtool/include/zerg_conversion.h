#ifndef _H_FASTATOI__
#define _H_FASTATOI__

#include <cmath>
#include <cstring>

namespace ztool {

inline int fast_atoi(const char *buff) {
    int c = 0, sign = 0, x = 0;
    const char *p = buff;
    // eat whitespaces and check sign
    for (c = *(p++); (c < 48 || c > 57); c = *(p++)) {
        if (c == 45) {
            sign = 1;
            c = *(p++);
            break;
        }
    };
    for (; c > 47 && c < 58; c = *(p++)) x = (x << 1) + (x << 3) + c - 48;
    return sign ? -x : x;
}

}  // namespace ztool
#endif
