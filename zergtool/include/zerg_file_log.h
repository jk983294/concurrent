#ifndef ZERGFILELOG_H
#define ZERGFILELOG_H

#include <sys/time.h>
#include <cstdarg>
#include <string>

namespace ztool {
struct ZergFileLog {
    ZergFileLog() = default;

    explicit ZergFileLog(const std::string &filename) { SetLogFile(filename); }

    ~ZergFileLog() { Close(); }

    void SetLogFile(const std::string &filename) {
        if (m_fp != nullptr) fclose(m_fp);
        m_fp = fopen(filename.c_str(), "w+");
        if (m_fp == nullptr) {
            fprintf(stderr, "cannot open %s for writing\n", filename.c_str());
            return;
        }
        m_bLogInited = true;
    }

    void Write(const char *format, ...) {
        if (!m_bLogInited || m_fp == nullptr) {
            return;
        }
        va_list args;
        va_start(args, format);
        vsprintf(content, format, args);
        va_end(args);

        if (m_bShowTime) {
            struct timeval raw_time {};
            gettimeofday(&raw_time, nullptr);
            struct tm *time_info;
            char buffer[80];
            time_info = localtime(&raw_time.tv_sec);
            strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", time_info);
            if (m_bAutoEndLine)
                fprintf(m_fp, "[%s.%03ld] %s\n", buffer, (raw_time.tv_usec % 1000000) / 1000, content);
            else
                fprintf(m_fp, "[%s.%03ld] %s", buffer, (raw_time.tv_usec % 1000000) / 1000, content);
        } else {
            if (m_bAutoEndLine)
                fprintf(m_fp, "%s\n", content);
            else
                fprintf(m_fp, "%s", content);
        }
        fflush(m_fp);
    }

    void Flush() {
        if (m_fp != nullptr) fflush(m_fp);
    }

    /// show time or not
    void SetShowTimeStamp(bool show) { m_bShowTime = show; }
    void SetAutoEndLine(bool end_line) { m_bAutoEndLine = end_line; }

    void Close() {
        if (m_fp != nullptr) {
            Flush();
            fclose(m_fp);
        }
        m_fp = nullptr;
        m_bLogInited = false;
    }

    char content[2048];
    FILE *m_fp{nullptr};
    bool m_bLogInited{false};
    bool m_bShowTime{true};
    bool m_bAutoEndLine{true};
};
}  // namespace ztool

#endif
