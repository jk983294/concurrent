#include "Benchmark.h"
#include "allocator/PoolAllocator.h"

using namespace std;

int main() {
    constexpr std::size_t batchSize = 1024;
    Performance<frenzy::PoolAllocator<int, batchSize>> performance;
    performance.run();

    Performance<frenzy::FastPoolAllocator<int>> perf1;
    perf1.run();

    MapPerformance<frenzy::PoolAllocator<std::pair<const int, int>, batchSize>> mperf;
    mperf.run();

    MapPerformance<frenzy::FastPoolAllocator<std::pair<const int, int>>> mperf1;
    mperf1.run();
    return 0;
}
