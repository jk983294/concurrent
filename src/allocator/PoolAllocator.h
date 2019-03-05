#ifndef CONCURRENT_POOL_ALLOCATOR_H
#define CONCURRENT_POOL_ALLOCATOR_H

#include <cstdint>
#include <cstdlib>
#include <memory>

using namespace std;

namespace frenzy {

/**
 * not thread safe, suit for list, not good for map, set
 * do not support vector
 */
template <class T, std::size_t BatchSize = 1024>
class MemoryPool {
    struct Block {
        Block *next;
    };

    class Buffer {
        static const std::size_t blockSize = sizeof(T) > sizeof(Block) ? sizeof(T) : sizeof(Block);
        uint8_t data[blockSize * BatchSize];

    public:
        Buffer *const next;

        Buffer(Buffer *next_) : next(next_) {}

        T *getBlock(std::size_t index) { return reinterpret_cast<T *>(&data[blockSize * index]); }
    };

    Block *firstFreeBlock = nullptr;  // one block for one T element
    Buffer *firstBuffer = nullptr;    // one buffer contains BatchSize block
                                      /**
                                       * when index >= BatchSize, allocate new buffer, otherwise firstBuffer->getBlock(bufferBlockIndex++)
                                       */
    std::size_t bufferBlockIndex = BatchSize;

    MemoryPool(MemoryPool &&memoryPool) = delete;
    MemoryPool(const MemoryPool &memoryPool) = delete;
    MemoryPool &operator=(MemoryPool &&memoryPool) = delete;
    MemoryPool &operator=(const MemoryPool &memoryPool) = delete;

public:
    MemoryPool() = default;

    ~MemoryPool() {
        while (firstBuffer) {
            Buffer *buffer = firstBuffer;
            firstBuffer = buffer->next;
            delete buffer;
        }
    }

    T *allocate() {
        if (firstFreeBlock) {
            Block *block = firstFreeBlock;
            firstFreeBlock = block->next;
            return reinterpret_cast<T *>(block);
        }

        if (bufferBlockIndex >= BatchSize) {
            firstBuffer = new Buffer(firstBuffer);
            bufferBlockIndex = 0;
        }
        return firstBuffer->getBlock(bufferBlockIndex++);
    }

    /**
     * return block to its list's head
     */
    void deallocate(T *pointer) {
        Block *block = reinterpret_cast<Block *>(pointer);
        block->next = firstFreeBlock;
        firstFreeBlock = block;
    }
};

template <class T, std::size_t BatchSize = 1024>
class PoolAllocator : private MemoryPool<T, BatchSize> {
public:
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T value_type;

    template <class U>
    struct rebind {
        typedef PoolAllocator<U, BatchSize> other;
    };

    PoolAllocator() = default;

    pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0) {
        if (n != 1 || hint) throw std::bad_alloc();
        return MemoryPool<T, BatchSize>::allocate();
    }

    void deallocate(pointer p, size_type n) { MemoryPool<T, BatchSize>::deallocate(p); }

    void construct(pointer p, const_reference val) { new (p) T(val); }

    void destroy(pointer p) { p->~T(); }
};
}  // namespace frenzy

#endif
