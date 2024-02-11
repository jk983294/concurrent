#pragma once

#include <fststore.h>
#include <fsttable.h>
#include <columnfactory.h>
#include <zerg_file.h>

namespace ztool {
inline bool write_fst(std::string path_, size_t nrow, const std::vector<OutputColumnOption>& cols) {
    path_ = FileExpandUser(path_);
    std::vector<IntVectorAdapter*> ints;
    std::vector<DoubleVectorAdapter*> doubles;
    std::vector<LogicalVectorAdapter*> bools;
    std::vector<StringColumn*> strs;
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
        } else if (option.type == 4) {
            std::vector<std::string>& sVec = *reinterpret_cast<std::vector<std::string>*>(option.data);
            auto* pVec = new StringColumn(sVec);
            table.SetStringColumn(pVec, i);
            strs.push_back(pVec);
        } else if (option.type == 5) {
            const std::vector<bool>& bVec = *reinterpret_cast<const std::vector<bool>*>(option.data);
            auto* pVec = new LogicalVectorAdapter(nrow);
            for (size_t j = 0; j < nrow; ++j) {
                (pVec->Data())[j] = bVec[j]; // 00 = false, 01 = true and 10 = NA
            }
            table.SetLogicalColumn(pVec, i);
            bools.push_back(pVec);
        } else {
            throw std::runtime_error("write_fst un support type");
        }
    }

    StringArray colNames(colnames);
    table.SetColumnNames(colNames);
    FstStore fstStore(path_);
    fstStore.fstWrite(table, 30);
    printf("write %s success\n", path_.c_str());

    for (auto* vec : ints) delete vec;
    for (auto* vec : doubles) delete vec;
    for (auto* vec : bools) delete vec;
    for (auto* vec : strs) delete vec;
    return true;
}

struct FstReader {
    FstReader() = default;
    static void read(std::string path_, InputData& id) {
        path_ = FileExpandUser(path_);
        FstTable table;
        ColumnFactory columnFactory;
        std::vector<int> keyIndex;
        StringArray selectedCols;

        std::unique_ptr<StringColumn> col_names(new StringColumn());
        FstStore fstStore(path_);
        fstStore.fstRead(table, nullptr, 1, -1, &columnFactory, keyIndex, &selectedCols, &*col_names);

        id.rows = table.NrOfRows();
        for (uint64_t i = 0; i < selectedCols.Length(); ++i) {
            std::shared_ptr<DestructableObject> column;
            FstColumnType type;
            std::string colName;
            short int colScale;
            std::string annotation;
            table.GetColumn(i, column, type, colName, colScale, annotation);

            if (type == FstColumnType::INT_32) {
                auto i_vec = std::dynamic_pointer_cast<IntVector>(column);
                auto ptr = i_vec->Data();
                auto* pVec = new std::vector<int>(ptr, ptr + id.rows);
                id.ints.push_back(pVec);
                id.cols.push_back({3, pVec, selectedCols.GetElement(i)});
            } else if (type == FstColumnType::DOUBLE_64) {
                auto i_vec = std::dynamic_pointer_cast<DoubleVector>(column);
                auto ptr = i_vec->Data();
                auto* pVec = new std::vector<double>(ptr, ptr + id.rows);
                id.doubles.push_back(pVec);
                id.cols.push_back({1, pVec, selectedCols.GetElement(i)});
            } else if (type == FstColumnType::CHARACTER) {
                auto i_vec = std::dynamic_pointer_cast<StringVector>(column);
                auto ptr = i_vec->StrVec();
                auto* pVec = new std::vector<std::string>(*ptr);
                id.strs.push_back(pVec);
                id.cols.push_back({4, pVec, selectedCols.GetElement(i)});
            } else if (type == FstColumnType::BOOL_2) {
                auto i_vec = std::dynamic_pointer_cast<IntVector>(column);
                auto ptr = i_vec->Data();
                auto* pVec = new std::vector<bool>(id.rows, false);
                for (uint64_t j = 0; j < id.rows; ++j) {
                    if (ptr[j] == 1) (*pVec)[j] = true; // 00 = false, 01 = true and 10 = NA
                }
                id.bools.push_back(pVec);
                id.cols.push_back({5, pVec, selectedCols.GetElement(i)});
            }
        }
    }
};
}  // namespace ztool