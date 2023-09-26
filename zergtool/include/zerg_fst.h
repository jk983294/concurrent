#pragma once

#include <fststore.h>
#include <fsttable.h>
#include <zerg_file.h>

namespace ztool {
bool write_fst(std::string path_, size_t nrow, const std::vector<OutputColumnOption>& cols) {
    path_ = FileExpandUser(path_);
    std::vector<IntVectorAdapter*> ints;
    std::vector<DoubleVectorAdapter*> doubles;
    std::vector<std::string> colnames;

    FstTable table(nrow);
    table.InitTable(cols.size(), nrow);
    for (size_t i = 0; i < cols.size(); ++i) {
        auto& option = cols[i];
        colnames.push_back(option.name);
        if (option.type == 1) {
            auto* doubleVec = new DoubleVectorAdapter(reinterpret_cast<double*>(option.data));
            table.SetDoubleColumn(doubleVec, i);
            doubles.push_back(doubleVec);
        } else if (option.type == 2) {
            auto* vec = reinterpret_cast<float*>(option.data);
            auto* doubleVec = new DoubleVectorAdapter(nrow, FstColumnAttribute::DOUBLE_64_BASE, 0);
            double* raw_data = doubleVec->Data();
            for (size_t j = 0; j < nrow; ++j) {
                raw_data[j] = vec[j];
            }
            table.SetDoubleColumn(doubleVec, i);
            doubles.push_back(doubleVec);
        } else if (option.type == 3) {
            auto* pVec = new IntVectorAdapter(reinterpret_cast<int*>(option.data));
            table.SetIntegerColumn(pVec, i);
            ints.push_back(pVec);
        } else {
            throw std::runtime_error("write_fst un support type");
        }
    }

    StringArray colNames(colnames);
    table.SetColumnNames(colNames);
    FstStore fstStore(path_);
    fstStore.fstWrite(table, 30);
    printf("write %s success", path_.c_str());

    for (auto* vec : ints) delete vec;
    for (auto* vec : doubles) delete vec;
    return true;
}
}  // namespace ztool