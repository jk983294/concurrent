#ifndef _ZERG_EXCEPTION_H_
#define _ZERG_EXCEPTION_H_

#include <exception>
#include <sstream>
#include <string>

namespace zerg {

class ZergException : public std::exception {
public:
    std::string errorMsg;
    std::string fullMsg;
    std::string file;
    int line{0};

public:
    ZergException() = default;
    ZergException(const std::string& errorMsg_, const char* file_, int line_) noexcept
        : errorMsg(errorMsg_), file(file_), line(line_) {
        std::ostringstream oss;
        oss << errorMsg << " from " << file << ":" << line;
        fullMsg = oss.str();
    }
    ZergException(const ZergException& e) noexcept
        : errorMsg(e.errorMsg), fullMsg(e.fullMsg), file(e.file), line(e.line) {}
    ~ZergException() override = default;

    const char* what() const noexcept override { return fullMsg.c_str(); }
};
}  // namespace zerg

#define THROW_ZERG_EXCEPTION(data)                                    \
    {                                                                 \
        do {                                                          \
            std::ostringstream oss;                                   \
            oss << data;                                              \
            throw zerg::ZergException(oss.str(), __FILE__, __LINE__); \
        } while (false);                                              \
    }

#endif
