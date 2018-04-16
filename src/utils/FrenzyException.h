#ifndef CONCURRENT_FRENZY_EXCEPTION_H
#define CONCURRENT_FRENZY_EXCEPTION_H

#include <exception>
#include <sstream>
#include <string>

namespace frenzy {

class FrenzyException : public std::exception {
public:
    std::string errorMsg;
    std::string fullMsg;
    std::string file;
    int line;

public:
    FrenzyException() throw() {}
    FrenzyException(const std::string& errorMsg_, const char* file_, int line_)
        : errorMsg(errorMsg_), file(file_), line(line_) {
        std::ostringstream oss;
        oss << errorMsg << " from " << file << ":" << line;
        fullMsg = oss.str();
    }
    FrenzyException(const FrenzyException& e) : errorMsg(e.errorMsg), fullMsg(e.fullMsg), file(e.file), line(e.line) {}
    ~FrenzyException() {}

    const char* what() const throw() { return fullMsg.c_str(); }
};
}

#define THROW_FRENZY_EXCEPTION(data)                                  \
    {                                                                 \
        std::ostringstream oss;                                       \
        oss << data;                                                  \
        throw frenzy::FrenzyException(oss.str(), __FILE__, __LINE__); \
    }

#endif
