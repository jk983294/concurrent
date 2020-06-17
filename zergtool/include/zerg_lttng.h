#if USE_LTTNG
#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER zerg

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "zerg_lttng.h"

#if !defined(ZERG_LTTNG_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define ZERG_LTTNG_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(zerg, zerg_str_tp,
                 TP_ARGS(const char*, string_file, const char*, string_func, int, integer_line, const char*,
                         string_desc),
                 TP_FIELDS(ctf_string(file, string_file) ctf_string(func, string_func)
                               ctf_integer(int, line, integer_line) ctf_string(desc, string_desc)))

TRACEPOINT_EVENT(zerg, zerg_int_tp,
                 TP_ARGS(const char*, string_file, const char*, string_func, int, integer_line, int, user_int),
                 TP_FIELDS(ctf_string(file, string_file) ctf_string(func, string_func)
                               ctf_integer(int, line, integer_line) ctf_integer(int, index, user_int)))

#endif

#include <lttng/tracepoint-event.h>

#else
#define tracepoint(...) \
    {}
#endif /* USE_LTTNG */
