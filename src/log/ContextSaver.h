#ifndef CONTEXT_LOG_H
#define CONTEXT_LOG_H

#include <cstdint>
#include <cstring>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "media/ShmUtils.h"
#include "utils/Traits.h"

namespace frenzy {

struct ContextItem {
    TypeIndex typeIndex;
    void* address;
    size_t length;
};

struct ContextMeta {
    uint64_t magic{0x1234FFFF};
    uint64_t metaSize{sizeof(ContextMeta)};
    uint64_t dataSize{0};
    uint64_t used{0};
};

class ContextSaver {
public:
    ContextSaver(std::string fileName_, size_t len) : fileName(fileName_) {
        size_t shmSize = sizeof(ContextMeta) + len;
        start = (uint8_t*)create_mmap(fileName_, shmSize);
        if (start != nullptr) {
            initMemoryLayout(len);
        } else {
            std::abort();
        }
    }

    ContextSaver(size_t len = 1024 * 1024) {
        start = new uint8_t[sizeof(ContextMeta) + len];
        initMemoryLayout(len);
    }

    ~ContextSaver() {
        if (fileName.empty()) {
            delete[] start;
        }
    }

    ContextSaver& schedule(char& value) { return _schedule(value, TypeIndex::char_type); }
    ContextSaver& schedule(int& value) { return _schedule(value, TypeIndex::int_type); }
    ContextSaver& schedule(long& value) { return _schedule(value, TypeIndex::int64_type); }
    ContextSaver& schedule(float& value) { return _schedule(value, TypeIndex::float_type); }
    ContextSaver& schedule(double& value) { return _schedule(value, TypeIndex::double_type); }
    ContextSaver& schedule(std::string& value) { return _schedule(value, TypeIndex::string_type); }

    // size 0 means saver will evaluate by std::strlen every time, otherwise it will directly use size
    ContextSaver& schedule(char* value, size_t size = 0) {
        userDefinedContext.push_back(ContextItem{TypeIndex::char_array_type, static_cast<void*>(value), size});
        return *this;
    }

    ContextSaver& save() {
        for (const auto& var : userDefinedContext) {
            if (var.typeIndex == TypeIndex::string_type) {
                std::string* pStr = (std::string*)var.address;
                save(pStr->c_str(), pStr->size());
            } else if (var.typeIndex == TypeIndex::char_array_type) {
                save((char*)var.address, var.length);
            } else {
                *writePos++ = var.typeIndex;
                memcpy(writePos, var.address, var.length);
                writePos += var.length;
            }
        }
        return *this;
    }

    ContextSaver& save(char value) { return _save(value, TypeIndex::char_type); }
    ContextSaver& save(int value) { return _save(value, TypeIndex::int_type); }
    ContextSaver& save(long value) { return _save(value, TypeIndex::int64_type); }
    ContextSaver& save(float value) { return _save(value, TypeIndex::float_type); }
    ContextSaver& save(double value) { return _save(value, TypeIndex::double_type); }

    ContextSaver& save(const char* value, size_t len = 0) {
        *writePos++ = TypeIndex::string_type;
        if (value != nullptr && len == 0) {
            len = std::strlen(value);
        }
        memcpy(writePos, &len, sizeof(size_t));
        writePos += sizeof(size_t);
        if (value != nullptr) {
            memcpy(writePos, value, len);
            writePos += len;
        }
        return *this;
    }

    ContextSaver& load(char& value) { return _load(value, TypeIndex::char_type); }
    ContextSaver& load(int& value) { return _load(value, TypeIndex::int_type); }
    ContextSaver& load(long& value) { return _load(value, TypeIndex::int64_type); }
    ContextSaver& load(float& value) { return _load(value, TypeIndex::float_type); }
    ContextSaver& load(double& value) { return _load(value, TypeIndex::double_type); }

    ContextSaver& load(char* value) {
        if ((checkTypeNoThrow(TypeIndex::string_type) || checkTypeNoThrow(TypeIndex::char_array_type)) &&
            value != nullptr) {
            ++readPos;
            size_t len = 0;
            memcpy(&len, readPos, sizeof(size_t));
            readPos += sizeof(size_t);
            if (len > 0) {
                memcpy(value, readPos, len);
                value[len] = '\0';
                readPos += len;
            }
        }
        return *this;
    }

    ContextSaver& load(std::string& value) {
        if (checkTypeNoThrow(TypeIndex::string_type) || checkTypeNoThrow(TypeIndex::char_array_type)) {
            ++readPos;
            size_t len = 0;
            memcpy(&len, readPos, sizeof(size_t));
            readPos += sizeof(size_t);
            if (len > 0) {
                value.resize(len + 1);
                memcpy(value.data(), readPos, len);
                value[len] = '\0';
                readPos += len;
            } else {
                value = "";
            }
        }
        return *this;
    }

private:
    void initMemoryLayout(size_t len) {
        contextMeta = reinterpret_cast<ContextMeta*>(start);
        readPos = writePos = start + sizeof(ContextMeta);
        ContextMeta dummy;
        dummy.dataSize = len;
        *contextMeta = dummy;
    }

    template <typename T>
    ContextSaver& _schedule(T& value, TypeIndex typeIndex) {
        userDefinedContext.push_back({typeIndex, &value, sizeof(T)});
        return *this;
    }

    template <typename T>
    ContextSaver& _save(const T& value, TypeIndex typeIndex) {
        *writePos++ = typeIndex;
        memcpy(writePos, &value, sizeof(T));
        writePos += sizeof(T);
        return *this;
    }

    template <typename T>
    ContextSaver& _load(T& value, TypeIndex typeIndex) {
        if (checkType(typeIndex)) {
            ++readPos;
            memcpy(&value, readPos, sizeof(T));
            readPos += sizeof(T);
        }
        return *this;
    }

    bool checkType(TypeIndex typeIndex) {
        if (*readPos != typeIndex) {
            std::ostringstream oss;
            oss << "type not match " << typeIndex << " <--> " << *readPos << std::endl;
            return false;
        }
        return true;
    }

    bool checkTypeNoThrow(TypeIndex typeIndex) { return *readPos == typeIndex; }

private:
    std::string fileName;
    ContextMeta* contextMeta{nullptr};
    uint8_t* start{nullptr};
    uint8_t* writePos{nullptr};
    uint8_t* readPos{nullptr};
    std::vector<ContextItem> userDefinedContext;
};

}  // namespace frenzy
#endif
