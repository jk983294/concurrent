#ifndef CONCURRENT_MPMC_BOUNDED_QUEUE_H
#define CONCURRENT_MPMC_BOUNDED_QUEUE_H

#include <atomic>
#include <stdexcept>
#include <type_traits>

namespace frenzy {

/**
 * push: loop wait until my turn (2 * (ticket / capacity)) to write slot (ticket % capacity)
 * then set turn = turn + 1 to inform the readers we are done pushing
 * the result slot's turn will be odd if that slot got written something in
 *
 * pop: loop wait until my turn (2 * (ticket / capacity) + 1) to read slot (ticket % capacity)
 * then turn = turn + 1 to inform the writers we are done reading
 * the result slot's turn will be even if that slot got read out
 */
template <typename T>
class MpmcBoundedQueue {
private:
    static constexpr size_t CacheLineSize = 128;

    struct Slot {
        ~Slot() noexcept {
            if (turn & 1) destroy();  // odd means something pushed but have not popped
        }

        template <typename... Args>
        void construct(Args &&... args) noexcept {
            static_assert(std::is_nothrow_constructible<T, Args &&...>::value,
                          "T must be nothrow constructible with Args&&...");
            new (&storage) T(std::forward<Args>(args)...);
        }

        void destroy() noexcept {
            static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");
            reinterpret_cast<T *>(&storage)->~T();
        }

        T &&move() noexcept { return reinterpret_cast<T &&>(storage); }

        // Align to avoid false sharing between adjacent slots
        alignas(CacheLineSize) std::atomic<size_t> turn = {0};
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    };

private:
    static_assert(std::is_nothrow_copy_assignable<T>::value || std::is_nothrow_move_assignable<T>::value,
                  "T must be nothrow copy or move assignable");
    static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible");

public:
    explicit MpmcBoundedQueue(const size_t capacity) : capacity_(capacity), head_(0), tail_(0) {
        if (capacity_ < 1) {
            throw std::invalid_argument("capacity < 1");
        }

        size_t space = capacity * sizeof(Slot) + CacheLineSize - 1;
        buf_ = malloc(space);
        if (buf_ == nullptr) {
            throw std::bad_alloc();
        }
        void *buf = buf_;
        slots_ = reinterpret_cast<Slot *>(std::align(CacheLineSize, capacity * sizeof(Slot), buf, space));

        if (slots_ == nullptr) {
            free(buf_);
            throw std::bad_alloc();
        }

        for (size_t i = 0; i < capacity_; ++i) {
            new (&slots_[i]) Slot();
        }
    }

    ~MpmcBoundedQueue() noexcept {
        for (size_t i = 0; i < capacity_; ++i) {
            slots_[i].~Slot();
        }
        free(buf_);
    }

    // non-copyable and non-movable
    MpmcBoundedQueue(const MpmcBoundedQueue &) = delete;
    MpmcBoundedQueue &operator=(const MpmcBoundedQueue &) = delete;

    template <typename... Args>
    void emplace(Args &&... args) noexcept {
        static_assert(std::is_nothrow_constructible<T, Args &&...>::value,
                      "T must be nothrow constructible with Args&&...");
        auto const head = head_.fetch_add(1);  // head_ = head + 1, multiply producer can always get its head location
        auto &slot = slots_[idx(head)];

        while (turn(head) * 2 != slot.turn.load(std::memory_order_acquire))
            ;  // loop until this slot got consumed

        slot.construct(std::forward<Args>(args)...);
        slot.turn.store(turn(head) * 2 + 1, std::memory_order_release);  // turn + 1 to inform reader
    }

    template <typename... Args>
    bool try_emplace(Args &&... args) noexcept {
        static_assert(std::is_nothrow_constructible<T, Args &&...>::value,
                      "T must be nothrow constructible with Args&&...");
        auto head = head_.load(std::memory_order_acquire);
        for (;;) {
            auto &slot = slots_[idx(head)];
            if (turn(head) * 2 == slot.turn.load(std::memory_order_acquire)) {  // check if my turn
                if (head_.compare_exchange_strong(head, head + 1)) {
                    // if I consume this slot, then head_ value set to head + 1, others failed the contention
                    slot.construct(std::forward<Args>(args)...);
                    slot.turn.store(turn(head) * 2 + 1, std::memory_order_release);
                    return true;
                }
            } else {
                auto const prevHead = head;  // failed contention, head already set to head + 1, then quit here
                head = head_.load(std::memory_order_acquire);
                if (head == prevHead) {
                    return false;
                }
            }
        }
    }

    void push(const T &v) noexcept {
        static_assert(std::is_nothrow_copy_constructible<T>::value, "T must be nothrow copy constructible");
        emplace(v);
    }

    template <typename P, typename = typename std::enable_if<std::is_nothrow_constructible<T, P &&>::value>::type>
    void push(P &&v) noexcept {
        emplace(std::forward<P>(v));
    }

    bool try_push(const T &v) noexcept {
        static_assert(std::is_nothrow_copy_constructible<T>::value, "T must be nothrow copy constructible");
        return try_emplace(v);
    }

    template <typename P, typename = typename std::enable_if<std::is_nothrow_constructible<T, P &&>::value>::type>
    bool try_push(P &&v) noexcept {
        return try_emplace(std::forward<P>(v));
    }

    void pop(T &v) noexcept {
        auto const tail = tail_.fetch_add(1);
        auto &slot = slots_[idx(tail)];
        while (turn(tail) * 2 + 1 != slot.turn.load(std::memory_order_acquire))
            ;  // loop until this slot got produced
        v = slot.move();
        slot.destroy();
        slot.turn.store(turn(tail) * 2 + 2, std::memory_order_release);  // inform writer
    }

    bool try_pop(T &v) noexcept {
        auto tail = tail_.load(std::memory_order_acquire);
        for (;;) {
            auto &slot = slots_[idx(tail)];
            if (turn(tail) * 2 + 1 == slot.turn.load(std::memory_order_acquire)) {  // check if my turn
                if (tail_.compare_exchange_strong(tail, tail + 1)) {
                    v = slot.move();
                    slot.destroy();
                    slot.turn.store(turn(tail) * 2 + 2, std::memory_order_release);
                    return true;
                }
            } else {
                auto const prevTail = tail;
                tail = tail_.load(std::memory_order_acquire);
                if (tail == prevTail) {
                    return false;
                }
            }
        }
    }

private:
    constexpr size_t idx(size_t i) const noexcept { return i % capacity_; }

    constexpr size_t turn(size_t i) const noexcept { return i / capacity_; }

private:
    const size_t capacity_;
    Slot *slots_;
    void *buf_;

    // Align to avoid false sharing between head_ and tail_
    alignas(CacheLineSize) std::atomic<size_t> head_;
    alignas(CacheLineSize) std::atomic<size_t> tail_;
};
}

#endif
