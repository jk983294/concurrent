#include "Benchmark.h"
#include "allocator/PoolAllocator.h"

using namespace std;

int main() {
    constexpr std::size_t batchSize = 1024;
    Performance<frenzy::PoolAllocator<int, batchSize>> performance;
    performance.run();

    Performance<frenzy::FastPoolAllocator<int>> perf1;
    perf1.run();
    return 0;
}
