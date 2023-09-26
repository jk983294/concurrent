#include <zerg_fst.h>

using namespace ztool;

int main() {
    std::string path_ = "/tmp/data2.fst";
    int nrOfRows = 10;

    std::vector<double> d_vec(nrOfRows);
    std::vector<float> f_vec(nrOfRows);
    std::vector<int> i_vec(nrOfRows);
    for (int i = 0; i < nrOfRows; ++i) {
        d_vec[i] = i;
        f_vec[i] = i;
        i_vec[i] = i;
    }

    std::vector<OutputColumnOption> options;
    options.push_back({1, d_vec.data(), "my_double"});
    options.push_back({2, f_vec.data(), "my_float"});
    options.push_back({3, i_vec.data(), "my_int"});

    write_fst(path_, nrOfRows, options);
    return 0;
}
