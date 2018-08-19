#ifndef CONCURRENT_SHMUTILS_H
#define CONCURRENT_SHMUTILS_H

#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>

namespace frenzy {

    constexpr uint32_t PAGE_SIZE{4096};
    constexpr uint32_t META_SIZE{PAGE_SIZE};

    inline uint32_t _roundup_pagesize(uint32_t x_) { return (x_ + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1)); }

    void* create_mmap(const std::string& fileName, size_t& mapSize) {
        int fd = -1;
        mapSize = _roundup_pagesize(mapSize);
        fd = shm_open(fileName.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
        if (fd < 0) {
            // created under /dev/shm/
            std::cerr << "shm_open: " << strerror(errno) << " : " << fileName << std::endl;
            return nullptr;
        }
        if (ftruncate(fd, mapSize) < 0) {
            close(fd);
            shm_unlink(fileName.c_str());
            std::cerr << "ftruncate: " << strerror(errno) << " : " << fileName << std::endl;
            return nullptr;
        }

        struct stat stats;
        if (fstat(fd, &stats) < 0) {
            close(fd);
            shm_unlink(fileName.c_str());
            std::cerr << "fstat: " << strerror(errno) << " : " << fileName << std::endl;
            return nullptr;
        }
        size_t size = (mapSize != 0) ? mapSize : static_cast<size_t>(stats.st_size);
        if (static_cast<size_t>(stats.st_size) < size) {
            close(fd);
            shm_unlink(fileName.c_str());
            std::cerr << "map size exceed file size, " << size << " > " << stats.st_size << std::endl;
            return nullptr;
        }
        void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            close(fd);
            shm_unlink(fileName.c_str());
            std::cerr << "mmap: " << strerror(errno)<< std::endl;
            return nullptr;
        }
        return addr;
    }

    void* create_mmap_with_meta(const std::string& fileName, size_t& mapSize) {
        mapSize += META_SIZE;
        return create_mmap(fileName, mapSize);
    }

    void* attach_mmap(const std::string& fileName, size_t& mapSize) {
        int fd = -1;
        mapSize = _roundup_pagesize(mapSize);
        fd = shm_open(fileName.c_str(), O_RDWR, 0666);
        if (fd < 0) {
            std::cerr << "shm_open attach: " << strerror(errno) << " : " << fileName << std::endl;
        }

        struct stat stats;
        if (fstat(fd, &stats) < 0) {
            close(fd);
            std::cerr << "fstat: " << strerror(errno) << " : " << fileName << std::endl;
        }

        size_t size = (mapSize != 0) ? mapSize : static_cast<size_t>(stats.st_size);
        if (static_cast<size_t>(stats.st_size) < size) {
            close(fd);
            std::cerr << "map size exceed file size, " << size << " > " <<stats.st_size << std::endl;
        }
        void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            close(fd);
            std::cerr << "mmap: " << strerror(errno)<< std::endl;
        }
        return addr;
    }

    void* attach_mmap_with_meta(const std::string& fileName, size_t& mapSize) {
        mapSize += META_SIZE;
        return attach_mmap(fileName, mapSize);
    }

}

#endif
