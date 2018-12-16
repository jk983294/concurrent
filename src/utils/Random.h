#ifndef CONCURRENT_RANDOM_H
#define CONCURRENT_RANDOM_H

#include <array>
#include <cstdint>
#include <random>
#include <type_traits>

namespace frenzy {

class Random {
public:
    /**
     * Returns a random uint32_t in [0, max). If max == 0, returns 0.
     */
    static uint32_t rand32(uint32_t max) { return rand32(0, max); }

    /**
     * Returns a random uint32_t in [min, max) given a specific RNG.
     * If min == max, returns 0.
     */
    static uint32_t rand32(uint32_t min, uint32_t max) {
        if (min == max) {
            return 0;
        }
        std::random_device rd;
        std::mt19937 rng(rd());
        return std::uniform_int_distribution<uint32_t>(min, max - 1)(rng);
    }

    /**
     * Returns a random uint64_t in [0, max). If max == 0, returns 0.
     */
    static uint64_t rand64(uint64_t max) { return rand64(0, max); }

    /**
     * Returns a random uint64_t in [min, max). If min == max, returns 0.
     */
    static uint64_t rand64(uint64_t min, uint64_t max) {
        if (min == max) {
            return 0;
        }
        std::random_device rd;
        std::mt19937 rng(rd());
        return std::uniform_int_distribution<uint64_t>(min, max - 1)(rng);
    }
};
}  // namespace frenzy

#endif
