#ifndef CONCURRENT_RANDOM_H
#define CONCURRENT_RANDOM_H

#include <array>
#include <cstdint>
#include <random>
#include <type_traits>

namespace frenzy {

class ThreadLocalPRNG {
public:
    using result_type = uint32_t;

    result_type operator()();

    static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
};

class Random {
public:
    typedef std::mt19937 DefaultGenerator;

    /**
     * (Re-)Seed an existing RNG with a good seed.
     *
     * Note that you should usually use ThreadLocalPRNG unless you need
     * reproducibility (such as during a test), in which case you'd want
     * to create a RNG with a good seed in production, and seed it yourself
     * in test.
     */
    template <class RNG = DefaultGenerator>
    static void seed(RNG& rng);

    /**
     * Create a new RNG, seeded with a good seed.
     *
     * Note that you should usually use ThreadLocalPRNG unless you need
     * reproducibility (such as during a test), in which case you'd want
     * to create a RNG with a good seed in production, and seed it yourself
     * in test.
     */
    template <class RNG = DefaultGenerator>
    static RNG create();

    /**
     * Returns a random uint32_t
     */
    static uint32_t rand32() { return rand32(ThreadLocalPRNG()); }

    //        /**
    //         * Returns a random uint32_t given a specific RNG
    //         */
    //        template <class RNG>
    //        static uint32_t rand32(RNG&& rng) {
    //            return rng();
    //        }

    /**
     * Returns a random uint32_t in [0, max). If max == 0, returns 0.
     */
    static uint32_t rand32(uint32_t max) { return rand32(0, max, ThreadLocalPRNG()); }

    /**
     * Returns a random uint32_t in [0, max) given a specific RNG.
     * If max == 0, returns 0.
     */
    template <class RNG = ThreadLocalPRNG>
    static uint32_t rand32(uint32_t max, RNG&& rng) {
        return rand32(0, max, rng);
    }

    /**
     * Returns a random uint32_t in [min, max). If min == max, returns 0.
     */
    static uint32_t rand32(uint32_t min, uint32_t max) { return rand32(min, max, ThreadLocalPRNG()); }

    /**
     * Returns a random uint32_t in [min, max) given a specific RNG.
     * If min == max, returns 0.
     */
    template <class RNG = ThreadLocalPRNG>
    static uint32_t rand32(uint32_t min, uint32_t max, RNG&& rng) {
        if (min == max) {
            return 0;
        }
        return std::uniform_int_distribution<uint32_t>(min, max - 1)(rng);
    }

    /**
     * Returns a random uint64_t
     */
    static uint64_t rand64() { return rand64(ThreadLocalPRNG()); }

    //        /**
    //         * Returns a random uint64_t
    //         */
    //        template <class RNG = ThreadLocalPRNG>
    //        static uint64_t rand64(RNG&& rng) {
    //            return ((uint64_t)rng() << 32) | rng();
    //        }

    /**
     * Returns a random uint64_t in [0, max). If max == 0, returns 0.
     */
    static uint64_t rand64(uint64_t max) { return rand64(0, max, ThreadLocalPRNG()); }

    /**
     * Returns a random uint64_t in [0, max). If max == 0, returns 0.
     */
    template <class RNG = ThreadLocalPRNG>
    static uint64_t rand64(uint64_t max, RNG&& rng) {
        return rand64(0, max, rng);
    }

    /**
     * Returns a random uint64_t in [min, max). If min == max, returns 0.
     */
    static uint64_t rand64(uint64_t min, uint64_t max) { return rand64(min, max, ThreadLocalPRNG()); }

    /**
     * Returns a random uint64_t in [min, max). If min == max, returns 0.
     */
    template <class RNG = ThreadLocalPRNG>
    static uint64_t rand64(uint64_t min, uint64_t max, RNG&& rng) {
        if (min == max) {
            return 0;
        }
        return std::uniform_int_distribution<uint64_t>(min, max - 1)(rng);
    }
};
}

#endif
