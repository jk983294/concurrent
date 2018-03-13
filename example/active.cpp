#include <nonblock/Active.h>
#include <iostream>

using namespace std;

void test() {
    frenzy::Active active;
    for (int i = 0; i < 10; ++i) {
        active.submit([i] { std::cout << "step " << i << std::endl; });
    }
}

int main() {
    std::thread t(test);
    t.join();
    return 0;
}
