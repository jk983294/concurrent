#ifndef _ZERG_MACRO_H_
#define _ZERG_MACRO_H_

//#include <log/ShmLog.h>
#include "zerg_exception.h"

#define HAS_KEY(a, b) (((a).find(b)) != ((a).end()))
#define IS_ZERO(a) (fabs(a) < 1e-6)
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

#define HAS_ELE(a, b) ((std::find((a).begin(), (a).end(), b)) != ((a).end()))

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x))
#endif

#define LOG_AND_THROW(...)           \
    do {                             \
        char error[256];             \
        sprintf(error, __VA_ARGS__); \
        LOG_ERROR("%s", error);      \
        sleep(1);                    \
        THROW_ZERG_EXCEPTION(error); \
    } while (0)

#endif
