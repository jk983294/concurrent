#ifndef CONCURRENT_FAST_POOL_ALLOCATOR_H
#define CONCURRENT_FAST_POOL_ALLOCATOR_H

#include <boost/pool/pool_alloc.hpp>

namespace frenzy {

template <unsigned PreAllocSize>
class PreAllocUserPool {
private:
    static PreAllocUserPool pool;
    char* poolAddress;
    uint64_t poolAddressEnd;
    uint64_t poolAddressStart;
    uint64_t currentIndex;

public:
    typedef std::size_t size_type;  // the size of the largest object to be allocated
    typedef std::ptrdiff_t difference_type;

    static char* malloc(const size_type bytes) {
        uint64_t newIndex = pool.currentIndex + (uint64_t)bytes;

        if (newIndex < PreAllocSize) {
            char* result = pool.poolAddress + pool.currentIndex;
            pool.currentIndex = newIndex;
            return result;
        }

        return new (std::nothrow) char[bytes];
    }

    static void free(char* const block) {
        // normally it should not go here, PreAllocUserPool is used in boost::fast_pool_allocator
        // check if the memory is allocated in the buffer
        uint64_t addr = reinterpret_cast<uint64_t>(block);
        if (addr < pool.poolAddressEnd && addr >= pool.poolAddressStart) {
            // do not release memory, it should maintained in boost::fast_pool_allocator
            return;
        }
        delete[] block;
    }

    PreAllocUserPool(const PreAllocUserPool&) = delete;
    PreAllocUserPool& operator=(const PreAllocUserPool&) = delete;

private:
    PreAllocUserPool() {
        poolAddress = new char[PreAllocSize];
        currentIndex = 0;
        poolAddressStart = reinterpret_cast<uint64_t>(poolAddress);
        poolAddressEnd = reinterpret_cast<uint64_t>(poolAddress + PreAllocSize);
    }

    ~PreAllocUserPool() { delete[] poolAddress; }
};

template <unsigned PreAllocSize>
PreAllocUserPool<PreAllocSize> PreAllocUserPool<PreAllocSize>::pool;

typedef PreAllocUserPool<1024 * 1024 * 100> PreAllocUserPool_100M;

template <typename T>
class FastPoolAllocator : public boost::fast_pool_allocator<T, PreAllocUserPool_100M> {
public:
    template <typename U, typename... Args>
    inline void construct(U* p, Args&&... args) {
        new (p) U{std::forward<Args>(args)...};
    }

    template <typename U>
    struct rebind {
        typedef FastPoolAllocator<U> other;
    };

    FastPoolAllocator() : boost::fast_pool_allocator<T, PreAllocUserPool_100M>() {}

    template <typename U>
    FastPoolAllocator(const FastPoolAllocator<U>& other)
        : boost::fast_pool_allocator<T, PreAllocUserPool_100M>(other) {}
};
}

#endif
