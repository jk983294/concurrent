#ifndef CONCURRENT_GZBUFFERREADER_H
#define CONCURRENT_GZBUFFERREADER_H

#include <zlib.h>
#include <string>

/**
 * typical usage:

    GzBufferReader<T> reader;
    reader.open(path);
    if (!reader.open(path)) {
        cerr << "cannot open " << path << endl;
        return;
    }

    while (!reader.iseof()) {
        int num = reader.read();
        if (num < 0) {
            cerr << "error when reading " << path << endl;
            abort();
        }
        for (int i = 0; i < num; ++i) {
            const auto& data = reader.buffer[i];
            // work on data
        }
    }
 */

template <typename T>
struct GzBufferReader {
    T* buffer{nullptr};
    int len = 1000;
    gzFile zp{nullptr};
    std::string path;

    GzBufferReader() { init(); }
    GzBufferReader(int len_) : len{len_} { init(); }
    ~GzBufferReader() {
        delete[] buffer;
        if (zp) gzclose(zp);
    }

    bool open() { return open(path); }

    bool open(const std::string& path_) {
        if (&path != &path_) path = path_;
        if (zp) {
            gzclose(zp);
        }
        zp = gzopen(path_.c_str(), "rb");
        return zp != nullptr;
    }

    int read() {
        if (gzeof(zp)) return 0;
        int ret_val = gzread(zp, buffer, sizeof(T) * len);
        if (ret_val < 0) {
            return -1;
        }
        return ret_val / sizeof(T);
    }

    bool iseof() { return gzeof(zp); }

private:
    void init() { buffer = new T[len]; }
};

#endif
