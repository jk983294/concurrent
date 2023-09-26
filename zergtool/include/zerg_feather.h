#pragma once

#include <arrow/api.h>
#include <arrow/dataset/api.h>
#include <arrow/filesystem/api.h>
#include <arrow/io/file.h>
#include <arrow/ipc/writer.h>
#include <zerg_file.h>

namespace ztool {
bool write_feather(std::string path_, size_t nrow, const std::vector<OutputColumnOption>& cols) {
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
}  // namespace ztool