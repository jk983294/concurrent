#include <zerg_fst.h>

using namespace ztool;

int main() {
    std::string path_ = "/tmp/data2.fst";
    size_t nrOfRows = 10;

    std::vector<double> d_vec(nrOfRows);
    std::vector<float> f_vec(nrOfRows);
    std::vector<int> i_vec(nrOfRows);
    std::vector<std::string> s_vec(nrOfRows);
    std::vector<bool> b_vec(nrOfRows);
    for (size_t i = 0; i < nrOfRows; ++i) {
        d_vec[i] = i;
        f_vec[i] = i;
        i_vec[i] = i;
        s_vec[i] = std::to_string(i);
        b_vec[i] = i % 2 == 0;
    }

    std::vector<OutputColumnOption> options;
    options.push_back({1, d_vec.data(), "my_double"});
    options.push_back({2, f_vec.data(), "my_float"});
    options.push_back({3, i_vec.data(), "my_int"});
    options.push_back({4, &s_vec, "my_str"});
    options.push_back({5, &b_vec, "my_bool"});

    write_fst(path_, nrOfRows, options);

    FstReader reader;
    reader.read(path_);
    printf("rows=%zu\n", reader.rows);
    for (auto& col : reader.cols) {
        if (col.type == 1) {
            auto& vec = *reinterpret_cast<std::vector<double>*>(col.data);
            printf("col %s type=%d data=%f\n", col.name.c_str(), col.type, vec.front());
        } else if (col.type == 3) {
            auto& vec = *reinterpret_cast<std::vector<int>*>(col.data);
            printf("col %s type=%d data=%d\n", col.name.c_str(), col.type, vec.front());
        } else if (col.type == 4) {
            auto& vec = *reinterpret_cast<std::vector<std::string>*>(col.data);
            printf("col %s type=%d data=%s\n", col.name.c_str(), col.type, vec.front().c_str());
        } else if (col.type == 5) {
            auto& vec = *reinterpret_cast<std::vector<bool>*>(col.data);
            printf("col %s type=%d data=%d\n", col.name.c_str(), col.type, (int)vec.front());
        }
    }
    return 0;
}
