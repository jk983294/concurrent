#include <ConcurrentWrapper.h>
#include <iostream>
#include <numeric>
#include <queue>

using namespace std;

int main() {
    std::vector<int> v(10);
    frenzy::ConcurrentWrapper<std::vector<int>&> cv{v};
    auto f1 = cv.submit([](std::vector<int>& v) -> int { return std::accumulate(v.begin(), v.end(), 0); });

    auto f2 = cv.submit([](std::vector<int>& v) -> bool {
        std::iota(v.begin(), v.end(), 0);
        return true;
    });
    auto f3 = cv.submit([](std::vector<int>& v) { return std::accumulate(v.begin(), v.end(), 0); });

    auto f4 = cv.submit([](std::vector<int>& v) { return v.size(); });

    auto f5 = cv.submit([](std::vector<int>&) {});

    std::cout << std::boolalpha;
    std::cout << f1.get() << std::endl;
    std::cout << f2.get() << std::endl;
    std::cout << f3.get() << std::endl;
    std::cout << f4.get() << std::endl;
    return 0;
}
