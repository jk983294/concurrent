#pragma once

#include <arrow/api.h>
#include <arrow/dataset/api.h>
#include <arrow/filesystem/api.h>
#include <arrow/io/file.h>
#include <arrow/ipc/writer.h>
#include <zerg_file.h>

namespace ztool {
inline bool write_feather(std::string path_, size_t nrow, const std::vector<OutputColumnOption>& cols) {
    path_ = FileExpandUser(path_);
    std::vector<std::shared_ptr<arrow::Array>> arrays;
    arrow::SchemaBuilder sb_;
    for (size_t i = 0; i < cols.size(); ++i) {
        auto& option = cols[i];
        if (option.type == 1) {
            auto tmp_builder = std::make_shared<arrow::NumericBuilder<arrow::DoubleType>>();
            std::ignore = tmp_builder->AppendValues(reinterpret_cast<double*>(option.data), nrow);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::float64()));
        } else if (option.type == 2) {
            auto tmp_builder = std::make_shared<arrow::NumericBuilder<arrow::FloatType>>();
            std::ignore = tmp_builder->AppendValues(reinterpret_cast<float*>(option.data), nrow);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::float32()));
        } else if (option.type == 3) {
            auto tmp_builder = std::make_shared<arrow::NumericBuilder<arrow::Int32Type>>();
            std::ignore = tmp_builder->AppendValues(reinterpret_cast<int*>(option.data), nrow);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::int32()));
        } else if (option.type == 4) {
            auto tmp_builder = std::make_shared<arrow::StringBuilder>();
            std::vector<std::string>& sVec = *reinterpret_cast<std::vector<std::string>*>(option.data);
            std::ignore = tmp_builder->AppendValues(sVec);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::utf8()));
        } else if (option.type == 5) {
            auto tmp_builder = std::make_shared<arrow::BooleanBuilder>();
            const std::vector<bool>& bVec = *reinterpret_cast<const std::vector<bool>*>(option.data);
            std::ignore = tmp_builder->AppendValues(bVec);
            std::shared_ptr<arrow::Array> tmp_a;
            std::ignore = tmp_builder->Finish(&tmp_a);
            arrays.push_back(tmp_a);
            std::ignore = sb_.AddField(arrow::field(option.name, arrow::boolean()));
        } else {
            throw std::runtime_error("write_feather un support type");
        }
    }

    auto schema = sb_.Finish().ValueOrDie();
    std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, arrays);

    std::shared_ptr<arrow::fs::FileSystem> fs = std::make_shared<arrow::fs::LocalFileSystem>();
    auto output = fs->OpenOutputStream(path_).ValueOrDie();
    auto writer = arrow::ipc::MakeFileWriter(output.get(), table->schema()).ValueOrDie();
    arrow::Status status_ = writer->WriteTable(*table);
    if (!status_.ok()) {
        printf("write table %s failed, error=%s\n", path_.c_str(), status_.message().c_str());
        return false;
    }
    status_ = writer->Close();
    if (!status_.ok()) {
        printf("writer close %s failed, error=%s\n", path_.c_str(), status_.message().c_str());
        return false;
    } else {
        printf("write %s success\n", path_.c_str());
        return true;
    }
}

namespace {
inline void get_data_by_arrow_type_helper(double val, int& out) { out = static_cast<int>(std::lround(val)); }
inline void get_data_by_arrow_type_helper(int val, int& out) { out = val; }
inline void get_data_by_arrow_type_helper(int64_t val, int& out) { out = val; }
inline void get_data_by_arrow_type_helper(double val, double& out) { out = val; }
inline void get_data_by_arrow_type_helper(float val, double& out) { out = val; }

template <typename TIn, typename TOut>
inline void get_array_data(std::vector<TOut>& vec, std::shared_ptr<arrow::Table>& Tbl, int idx, int64_t rows) {
    vec.resize(rows);
    auto pChArray = Tbl->column(idx);
    int NChunks = pChArray->num_chunks();
    int i = 0;
    for (int n = 0; n < NChunks; n++) {
        auto pArray = pChArray->chunk(n);
        int64_t ArrayRows = pArray->length();
        auto pTypedArray = std::dynamic_pointer_cast<TIn>(pArray);
        const auto* pData = pTypedArray->raw_values();
        for (int64_t j = 0; j < ArrayRows; j++) {
            TOut tmp;
            get_data_by_arrow_type_helper(pData[j], tmp);
            vec[i++] = tmp;
        }
    }
}

inline void get_array_data_string(std::vector<std::string>& vec, std::shared_ptr<arrow::Table>& Tbl, int idx,
                                  int64_t rows) {
    vec.resize(rows);
    auto pChArray = Tbl->column(idx);
    int NChunks = pChArray->num_chunks();
    int i = 0;
    for (int n = 0; n < NChunks; n++) {
        std::shared_ptr<arrow::Array> pArray = pChArray->chunk(n);
        int64_t ArrayRows = pArray->length();
        auto pTypedArray = std::dynamic_pointer_cast<arrow::StringArray>(pArray);
        for (int64_t j = 0; j < ArrayRows; j++) {
            std::string tmp = pTypedArray->GetString(j);
            vec[i++] = tmp;
        }
    }
}

inline void get_array_data_bool(std::vector<bool>& vec, std::shared_ptr<arrow::Table>& Tbl, int idx, int64_t rows) {
    vec.resize(rows);
    auto pChArray = Tbl->column(idx);
    int NChunks = pChArray->num_chunks();
    int i = 0;
    for (int n = 0; n < NChunks; n++) {
        std::shared_ptr<arrow::Array> pArray = pChArray->chunk(n);
        int64_t ArrayRows = pArray->length();
        auto pTypedArray = std::dynamic_pointer_cast<arrow::BooleanArray>(pArray);
        for (int64_t j = 0; j < ArrayRows; j++) {
            vec[i++] = pTypedArray->Value(j);
        }
    }
}
}  // namespace

struct FeatherReader {
    FeatherReader() = default;
    static void read(std::string path_, InputData& id) {
        path_ = FileExpandUser(path_);
        auto format = std::make_shared<arrow::dataset::IpcFileFormat>();
        std::shared_ptr<arrow::fs::FileSystem> fs = std::make_shared<arrow::fs::LocalFileSystem>();
        std::string uri = "file://" + path_;
        auto factory =
            arrow::dataset::FileSystemDatasetFactory::Make(uri, format, arrow::dataset::FileSystemFactoryOptions())
                .ValueOrDie();
        auto dataset = factory->Finish().ValueOrDie();
        auto scan_builder = dataset->NewScan().ValueOrDie();
        auto scanner = scan_builder->Finish().ValueOrDie();
        std::shared_ptr<arrow::Table> table1 = scanner->ToTable().ValueOrDie();
        auto schema_ = table1->schema();
        auto col_cnt = table1->num_columns();
        id.rows = table1->num_rows();

        for (int i = 0; i < col_cnt; ++i) {
            auto f_ = schema_->field(i);
            auto type_ = f_->type()->name();

            if (type_ == "int32") {
                auto* pVec = new std::vector<int>();
                get_array_data<arrow::Int32Array, int>(*pVec, table1, i, id.rows);
                id.ints.push_back(pVec);
                id.cols.push_back({3, pVec, f_->name()});
            } else if (type_ == "double") {
                auto* pVec = new std::vector<double>();
                get_array_data<arrow::DoubleArray, double>(*pVec, table1, i, id.rows);
                id.doubles.push_back(pVec);
                id.cols.push_back({1, pVec, f_->name()});
            } else if (type_ == "float") {
                auto* pVec = new std::vector<double>();
                get_array_data<arrow::FloatArray, double>(*pVec, table1, i, id.rows);
                id.doubles.push_back(pVec);
                id.cols.push_back({1, pVec, f_->name()});
            } else if (type_ == "utf8") {
                auto* pVec = new std::vector<std::string>();
                get_array_data_string(*pVec, table1, i, id.rows);
                id.strs.push_back(pVec);
                id.cols.push_back({4, pVec, f_->name()});
            } else if (type_ == "bool") {
                auto* pVec = new std::vector<bool>();
                get_array_data_bool(*pVec, table1, i, id.rows);
                id.bools.push_back(pVec);
                id.cols.push_back({5, pVec, f_->name()});
            } else {
                printf("unknown %s,%s\n", f_->name().c_str(), type_.c_str());
            }
        }
    }
};
}  // namespace ztool