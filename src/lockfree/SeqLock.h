#ifndef CONCURRENT_SEQLOCK_H
#define CONCURRENT_SEQLOCK_H

#include <atomic>
#include <type_traits>

namespace frenzy {

/**
 * more efficient than traditional read-write locks for the situation where there are many readers and few writers
 * reader never blocks, they use optimistic lock, if sequence not changed, then read success
 * writer must coordinate with mutex, it first advance sequence, then do write action, then advance sequence again
 */
template <typename T>
class SeqLock {
public:
    static_assert(std::is_nothrow_copy_assignable<T>::value, "T must satisfy is_nothrow_copy_assignable");
    static_assert(std::is_trivially_copy_assignable<T>::value, "T must satisfy is_trivially_copy_assignable");

    SeqLock() = default;
    SeqLock(T t) : value_(t) {}

    T load() const noexcept {
        T copy;
        std::size_t seq0, seq1;
        do {
            seq0 = seq_.load(std::memory_order_acquire);
            std::atomic_signal_fence(std::memory_order_acq_rel);
            copy = value_;
            std::atomic_signal_fence(std::memory_order_acq_rel);
            seq1 = seq_.load(std::memory_order_acquire);
        } while (seq0 != seq1 || seq0 & 1);
        return copy;
    }

    void store(const T &desired) noexcept {
        std::size_t seq0 = seq_.load(std::memory_order_relaxed);
        seq_.store(seq0 + 1, std::memory_order_release);
        std::atomic_signal_fence(std::memory_order_acq_rel);
        value_ = desired;
        std::atomic_signal_fence(std::memory_order_acq_rel);
        seq_.store(seq0 + 2, std::memory_order_release);
    }

private:
    static const std::size_t FalseSharingRange = 128;

    // align to prevent false sharing with adjacent data
    alignas(FalseSharingRange) T value_;
    std::atomic<std::size_t> seq_{0};

    // padding to prevent false sharing with adjacent data
    char padding_[FalseSharingRange - ((sizeof(value_) + sizeof(seq_)) % FalseSharingRange)];
    static_assert(((sizeof(value_) + sizeof(seq_) + sizeof(padding_)) % FalseSharingRange) == 0,
                  "sizeof(SeqLock<T>) should be a multiple of FalseSharingRange");
};
}

#endif
