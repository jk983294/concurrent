#ifndef CONCURRENT_ATOMIC_HASH_ARRAY_H
#define CONCURRENT_ATOMIC_HASH_ARRAY_H

#include <atomic>
#include <cstdint>
#include <cstring>
#include <random>
#include "Utils.h"
#include "utils/Random.h"

namespace frenzy {

/**
 * took from Folly AtomicUnorderedMap
 *
 * LIMITATIONS:
 * 1. Insert only (*) - the only write operation supported is findOrConstruct.
 * you can roll your own concurrency control for in-place updates of values
 * Inserted values won't be moved, but no concurrency control is provided for safely updating them.
 * 2. No resizing - you must specify the capacity up front,
 * Insert performance will degrade once the load factor is high. you can't remove existing keys.
 * 3. 2^30 maximum default capacity
 *
 * WHAT YOU GET IN EXCHANGE:
 * 1. Arbitrary key and value types - any K and V that can be used in a std::unordered_map
 * can be used here.  In fact, the key and value types don't even have to be copyable or movable!
 * 2. Keys and values in the map won't be moved -
 * it is safe to keep pointers or references to the keys and values in the map,
 * because they are never moved or destroyed (until the map itself is destroyed).
 * 3. Iterators are never invalidated - writes don't invalidate iterators, so you can scan and insert in parallel.
 * 4. Fast wait-free reads - reads are usually only a single cache miss, even when the hash table is very large.
 * Wait-freedom means that you won't see latency outliers even in the face of concurrent writes.
 * 5. Lock-free insert - writes proceed in parallel.
 * If a thread in the middle of a write is unlucky and gets suspended, it doesn't block anybody else.
 */
template <typename Key, typename Value, template <typename> class Atom = std::atomic, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          bool SkipKeyValueDeletion =
              (std::is_trivially_destructible<Key>::value && std::is_trivially_destructible<Value>::value),
          typename Allocator = std::allocator<char>>
struct AtomicHashMap {
    typedef Key key_type;
    typedef Value mapped_type;
    typedef std::pair<Key, Value> value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef Hash hasher;
    typedef KeyEqual key_equal;
    typedef const value_type& const_reference;

    struct ConstIterator {
        ConstIterator(const AtomicHashMap& owner, uint32_t slot) : owner_(owner), slot_(slot) {}

        ConstIterator(const ConstIterator&) = default;
        ConstIterator& operator=(const ConstIterator&) = default;

        const value_type& operator*() const { return owner_.slots_[slot_].keyValue(); }

        const value_type* operator->() const { return &owner_.slots_[slot_].keyValue(); }

        const ConstIterator& operator++() {  // pre increment
            while (slot_ > 0) {
                --slot_;
                if (owner_.slots_[slot_].state() == BucketState::LINKED) {
                    break;
                }
            }
            return *this;
        }

        ConstIterator operator++(int) {  // post increment
            auto prev = *this;
            ++*this;
            return prev;
        }

        bool operator==(const ConstIterator& rhs) const { return slot_ == rhs.slot_; }
        bool operator!=(const ConstIterator& rhs) const { return !(*this == rhs); }

    private:
        const AtomicHashMap& owner_;
        uint32_t slot_;
    };

    typedef ConstIterator const_iterator;
    friend ConstIterator;

private:
    static constexpr uint32_t kMaxAllocationTries = 1000;  // after this we throw

    enum BucketState : uint32_t {
        EMPTY = 0,
        CONSTRUCTING = 1,
        LINKED = 2,
    };

    /**
     * Lock-free insertion is easiest by pre-pending to collision chains.
     * A large chaining hash table takes two cache misses instead of one, however.
     * Our solution is to co-locate the bucket storage and the head storage,
     * so that even though we are traversing chains we are likely to stay within the same cache line.
     * Just make sure to traverse head before looking at any keys.
     * This strategy gives us 32 bit pointers and fast iteration.
     */
    struct Slot {
        /**
         * The bottom two bits are the BucketState, the rest is the index of the first bucket for the chain
         * whose keys map to this slot. When things are going well the head usually links to this slot,
         * but that doesn't always have to happen.
         */
        Atom<uint32_t> headAndState_;

        uint32_t next_;  // The next bucket in the chain

        typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type raw_;  // Key and Value

        ~Slot() {
            auto s = state();
            if (s == BucketState::LINKED) {
                keyValue().first.~Key();
                keyValue().second.~Value();
            }
        }

        BucketState state() const { return BucketState(headAndState_.load(std::memory_order_acquire) & 3); }

        void stateUpdate(BucketState before, BucketState after) { headAndState_ += (after - before); }

        value_type& keyValue() { return *static_cast<value_type*>(static_cast<void*>(&raw_)); }

        const value_type& keyValue() const { return *static_cast<const value_type*>(static_cast<const void*>(&raw_)); }
    };

public:
    /**
     * Constructs a map that will support the insertion of maxSize key-value pairs without
     * exceeding the max load factor. The capacity is limited to 2^30 for the default uint32_t
     */
    explicit AtomicHashMap(size_t maxSize, float maxLoadFactor = 0.8f, const Allocator& alloc = Allocator())
        : allocator_(alloc) {
        size_t capacity = size_t(maxSize / std::min(1.0f, maxLoadFactor) + 128);
        size_t avail = size_t{1} << (8 * sizeof(uint32_t) - 2);
        if (capacity > avail && maxSize < avail) {
            capacity = avail;
        }

        if (capacity < maxSize || capacity > avail) {
            throw std::invalid_argument("AtomicHashMap capacity must fit in uint32_t with 2 bits left over");
        }

        numSlots_ = capacity;
        slotMask_ = nextPowerOf2(capacity * 4) - 1;
        memoryRequested_ = sizeof(Slot) * capacity;
        slots_ = reinterpret_cast<Slot*>(allocator_.allocate(memoryRequested_));
        std::memset(reinterpret_cast<void*>(slots_), 0, memoryRequested_);
        // mark the zero-th slot as in-use but not valid, since that happens to be our nil value
        slots_[0].stateUpdate(BucketState::EMPTY, BucketState::CONSTRUCTING);
    }

    ~AtomicHashMap() {
        if (!SkipKeyValueDeletion) {
            for (size_t i = 1; i < numSlots_; ++i) {
                slots_[i].~Slot();
            }
        }
        allocator_.deallocate(reinterpret_cast<char*>(slots_), memoryRequested_);
    }

    /**
     * If it is not found calls the functor Func with a void* argument
     * that is raw storage suitable for placement construction of a Value (see raw_value_type),
     * then returns (iter,true).  It will return (iter,false) if there are other concurrent writes,
     * in which case the newly constructed value will be immediately destroyed.
     *
     * This function does not block other readers or writers.  If there are other concurrent writes,
     * many parallel calls to func may happen and only the first one to complete will win.
     * The values constructed by the other calls to func will be destroyed.
     * @tparam Func
     * @param key
     * @param func
     * @return (iter,false) if it is found.
     */
    template <typename Func>
    std::pair<const_iterator, bool> findOrConstruct(const Key& key, Func&& func) {
        auto const slot = keyToSlotIndex(key);
        auto prev = slots_[slot].headAndState_.load(std::memory_order_acquire);

        auto existing = find(key, slot);
        if (existing != 0) {
            return std::make_pair(ConstIterator(*this, existing), false);
        }

        auto idx = allocateNear(slot);
        new (&slots_[idx].keyValue().first) Key(key);
        func(static_cast<void*>(&slots_[idx].keyValue().second));

        while (true) {
            slots_[idx].next_ = prev >> 2;

            // merge the head update and the BucketState::CONSTRUCTING -> BucketState::LINKED update into a single CAS
            // if slot == idx
            auto after = idx << 2;
            if (slot == idx) {
                after += BucketState::LINKED;
            } else {
                after += (prev & 3);
            }

            if (slots_[slot].headAndState_.compare_exchange_strong(prev, after)) {  // success
                if (idx != slot) {
                    slots_[idx].stateUpdate(BucketState::CONSTRUCTING, BucketState::LINKED);
                }
                return std::make_pair(ConstIterator(*this, idx), true);
            }
            // compare_exchange_strong updates its first arg on failure, so there is no need to reread prev

            existing = find(key, slot);
            if (existing != 0) {
                // our allocated key and value are no longer needed
                slots_[idx].keyValue().first.~Key();
                slots_[idx].keyValue().second.~Value();
                slots_[idx].stateUpdate(BucketState::CONSTRUCTING, BucketState::EMPTY);

                return std::make_pair(ConstIterator(*this, existing), false);
            }
        }
    }

    template <class K, class V>
    std::pair<const_iterator, bool> emplace(const K& key, V&& value) {
        return findOrConstruct(key, [&](void* raw) { new (raw) Value(std::forward<V>(value)); });
    }

    const_iterator find(const Key& key) const { return ConstIterator(*this, find(key, keyToSlotIndex(key))); }

    const_iterator cbegin() const {
        auto slot = static_cast<uint32_t>(numSlots_ - 1);
        while (slot > 0 && slots_[slot].state() != BucketState::LINKED) {
            --slot;
        }
        return ConstIterator(*this, slot);
    }

    const_iterator cend() const { return ConstIterator(*this, 0); }

private:
    // manually manage the slot memory so we can bypass initialization and optionally destruction of the slots
    size_t memoryRequested_;
    size_t numSlots_;
    size_t slotMask_;  // tricky, see keyToSlotIndex
    Allocator allocator_;
    Slot* slots_;

private:
    uint32_t keyToSlotIndex(const Key& key) const {
        size_t h = hasher()(key);
        h &= slotMask_;
        while (h >= numSlots_) {
            h -= numSlots_;
        }
        return static_cast<uint32_t>(h);
    }

    uint32_t find(const Key& key, uint32_t slot) const {
        KeyEqual ke = {};
        auto hs = slots_[slot].headAndState_.load(std::memory_order_acquire);
        for (slot = hs >> 2; slot != 0; slot = slots_[slot].next_) {
            if (ke(key, slots_[slot].keyValue().first)) {
                return slot;
            }
        }
        return 0;
    }

    // allocates a slot and returns its index. tries to put it near slots_[start].
    uint32_t allocateNear(uint32_t start) {
        for (uint32_t tries = 0; tries < kMaxAllocationTries; ++tries) {
            auto slot = allocationAttempt(start, tries);
            auto prev = slots_[slot].headAndState_.load(std::memory_order_acquire);
            if ((prev & 3) == BucketState::EMPTY && slots_[slot].headAndState_.compare_exchange_strong(
                                                        prev, prev + BucketState::CONSTRUCTING - BucketState::EMPTY)) {
                return slot;
            }
        }
        throw std::bad_alloc();
    }

    /**
     * Returns the slot we should attempt to allocate after tries failed tries, starting from the specified slot.
     * This is pulled out so we can specialize it differently during deterministic testing
     */
    uint32_t allocationAttempt(uint32_t start, uint32_t tries) const {
        if (tries < 8 && start + tries < numSlots_) {
            return uint32_t(start + tries);
        } else {
            uint32_t rv = frenzy::Random::rand32(static_cast<uint32_t>(numSlots_));
            return rv;
        }
    }
};

/**
 * MutableAtom is a tiny wrapper than gives you the option of atomically updating values inserted into
 * an AtomicHashMap<K, MutableAtom<V>>.  This relies on AtomicHashMap's guarantee that it doesn't move values.
 */
template <typename T, template <typename> class Atom = std::atomic>
struct MutableAtom {
    mutable Atom<T> data;

    explicit MutableAtom(const T& init) : data(init) {}
};

/**
 * MutableData is a tiny wrapper than gives you the option of using an external concurrency control mechanism
 * to updating values inserted into an AtomicHashMap.
 */
template <typename T>
struct MutableData {
    mutable T data;
    explicit MutableData(const T& init) : data(init) {}
};

template <class T>
struct NonAtomic {
    T value;

    NonAtomic() = default;
    NonAtomic(const NonAtomic&) = delete;
    constexpr /* implicit */ NonAtomic(T desired) : value(desired) {}

    T operator+=(T arg) {
        value += arg;
        return load();
    }

    T load(std::memory_order /* order */ = std::memory_order_seq_cst) const { return value; }

    /* implicit */
    operator T() const { return load(); }

    void store(T desired, std::memory_order /* order */ = std::memory_order_seq_cst) { value = desired; }

    T exchange(T desired, std::memory_order /* order */ = std::memory_order_seq_cst) {
        T old = load();
        store(desired);
        return old;
    }

    bool compare_exchange_weak(T& expected, T desired, std::memory_order /* success */ = std::memory_order_seq_cst,
                               std::memory_order /* failure */ = std::memory_order_seq_cst) {
        if (value == expected) {
            value = desired;
            return true;
        }

        expected = value;
        return false;
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order /* success */ = std::memory_order_seq_cst,
                                 std::memory_order /* failure */ = std::memory_order_seq_cst) {
        if (value == expected) {
            value = desired;
            return true;
        }

        expected = value;
        return false;
    }

    bool is_lock_free() const { return true; }
};
}  // namespace frenzy

#endif
