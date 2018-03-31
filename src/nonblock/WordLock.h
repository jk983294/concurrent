#ifndef CONCURRENT_WORD_LOCK_H
#define CONCURRENT_WORD_LOCK_H

#include <x86intrin.h>
#include <cstdint>

namespace frenzy {
/**
 * single producer multiple consumer
 * at most 12 consumers, one bit one consumer
 */
class WordLock {
public:
    const static uint32_t updateTimeoutTSC = 5000;
    const static uint32_t LockMetaOffset = 12;

    struct MetaT {
        uint64_t readerCount : LockMetaOffset;
        uint64_t version : 64 - LockMetaOffset;
    };
    static_assert(sizeof(MetaT) == sizeof(uint64_t), "MetaT is not the same size as uint64_t");

    /**
     * loop until reader releases lock in the worst case,
     * this will loop until the readerCount in pMeta is 0
     * @param pMeta expect= {readerCount:0, version: v}, target={readerCount:0, version: v+1}
     */
    static void _increment_version_number_step1(uint64_t *pMeta) {
        uint64_t expected = __atomic_load_n(pMeta, __ATOMIC_RELAXED);
        uint64_t target;
        auto metaExpect = reinterpret_cast<MetaT *>(&expected);
        auto metaTarget = reinterpret_cast<MetaT *>(&target);

        bool timeout = false;
        const uint64_t baseTsc = __rdtsc();  // read time stamp counter
        uint64_t nowTsc = baseTsc;
        do {
            if (nowTsc > baseTsc + updateTimeoutTSC) {
                timeout = true;
                break;
            }
            nowTsc = __rdtsc();
            metaExpect->readerCount = 0;
            *metaTarget = *metaExpect;  // copy to target
            ++metaTarget->version;      // increment target version
            // check if *pMeta == *expected, if true, write target to *pMeta and return true
            // if false, then write expected = *pMeta, return false
        } while (!__atomic_compare_exchange_n(pMeta, &expected, target, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

        if (!timeout) {
            return;
        }
        metaExpect->readerCount = 0;
        ++metaExpect->version;
        __atomic_store_n(pMeta, expected, __ATOMIC_RELEASE);
    }

    /**
     * there is only one writer, so CAS is not necessary
     * given proper memory order to make sure reader sees the correct value
     */
    static void _increment_version_number_step2(uint64_t *pMeta) {
        uint64_t expected = __atomic_load_n(pMeta, __ATOMIC_RELAXED);
        auto m = reinterpret_cast<MetaT *>(&expected);
        ++m->version;
        __atomic_store_n(pMeta, expected, __ATOMIC_RELEASE);
    }

    /**
     * clear 1 << id flag for that consumer
     * @param pMeta
     * @param id
     */
    static void _unlock_reader(uint64_t *pMeta, uint32_t id) {
        uint64_t expected = __atomic_load_n(pMeta, __ATOMIC_RELAXED);
        uint64_t target = expected;
        auto m = reinterpret_cast<MetaT *>(&target);

        if (m->readerCount & (1 << id)) {  // client died here ...
            do {
                target = expected;
                // masking value with maximum value for 12 bits prevents conversion error
                m->readerCount = static_cast<uint64_t>(m->readerCount & ~(1 << id) & 0xFFF);
            } while (!__atomic_compare_exchange_n(pMeta, &expected, target, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));
        }
    }

    class VersionGuard {
    public:
        explicit VersionGuard(uint64_t *lock) : m_lock(lock) { _increment_version_number_step1(m_lock); }
        ~VersionGuard() { _increment_version_number_step2(m_lock); }

        VersionGuard(const VersionGuard &) = delete;
        VersionGuard(VersionGuard &&) = delete;
        VersionGuard &operator=(const VersionGuard &) = delete;
        VersionGuard &operator=(VersionGuard &&) = delete;

    private:
        uint64_t *m_lock = nullptr;
    };

    /**
     * only returns when version is even number and we successfully increment the reader count
     * Disable conversion checking here -- cannot cast to partial int
     */
    static bool _increment_reader_count(uint64_t *pMeta, uint32_t id, uint64_t &version, uint32_t nMaxAttemptsTsc) {
        uint64_t expected = __atomic_load_n(pMeta, __ATOMIC_RELAXED);
        uint64_t target;
        auto e = reinterpret_cast<MetaT *>(&expected);
        auto t = reinterpret_cast<MetaT *>(&target);

        bool timeout = false;
        const uint64_t baseTsc = __rdtsc();
        uint64_t nowTsc = baseTsc;
        // expect version not to be updating status, i.e. odd number
        do {
            if (nowTsc > baseTsc + nMaxAttemptsTsc) {
                timeout = true;
                break;
            }
            nowTsc = __rdtsc();
            // if even, keep as it is, otherwise advance to next even
            e->version = 0xFFFFFFFFFFFFF & ((e->version + 1) / 2 * 2);
            target = expected;
            // masking value with maximum value for 12 bits prevents conversion error functionally:
            // t.readerCount |= (1 << id);
            t->readerCount = static_cast<uint64_t>((t->readerCount | (1 << id)) & 0xFFF);
        } while (!__atomic_compare_exchange_n(pMeta, &expected, target, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

        if (timeout) {
            return false;
        }
        version = e->version;
        return true;
    }

    // atomic compare exchange, no need to check version number
    static void _decrement_reader_count(uint64_t *pMeta, uint32_t id, uint64_t &version) {
        uint64_t target;
        uint64_t expected = __atomic_load_n(pMeta, __ATOMIC_RELAXED);
        auto t = reinterpret_cast<MetaT *>(&target);

        // loop until reader releases lock in the worst case
        do {
            target = expected;
            // masking value with maximum value for 12 bits prevents conversion error functionally:
            // t.readerCount &= ~(1 << id);
            t->readerCount = static_cast<uint64_t>((t->readerCount & ~(1 << id)) & 0xFFF);
        } while (!__atomic_compare_exchange_n(pMeta, &expected, target, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));
        // writer may still write though if this client is slow.
        // we just need to check equalness, not necessary whether odd or even.
        version = t->version;
        return;
    }
};
}

#endif
