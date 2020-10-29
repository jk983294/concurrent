#include <getopt.h>
#include <ztool.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

char delimiter{','};
bool has_header{true};
bool sort_numerically{true};
string path{"/tmp/a.csv"};
string type{"sort"};
string head_line;
vector<int> field_idx;
vector<string> field_names;
vector<string> header_columns;
vector<string> contents;
string filter_condition;

void help() {
    cout << "Program options:" << endl;
    cout << "  -h                                    list help" << endl;
    cout << "  -p arg (=\"/tmp/a.csv\")              file path" << endl;
    cout << "  -t arg (=\"sort\")                    type (sort|extract|filter)" << endl;
    cout << "  -d arg (=\",\")                       delimiter" << endl;
    cout << "  -x                                    has header, when set, no header" << endl;
    cout << "  -y                                    sort mode, use text cmp instead of default numeric" << endl;
    cout << "  -n                                    column number, index from 0" << endl;
    cout << "  -f                                    field name" << endl;
    cout << "  -c                                    filter condition, like ('>5', '<3'), only support numeric, for "
            "text filter, use grep"
         << endl;
    cout << "usage:\n";
    cout << "  csvutil -t sort -p a.csv -f c" << endl;
    cout << "  csvutil -t sort -p a.csv -n 2" << endl;
    cout << "  csvutil -t extract -p a.csv -f a,c" << endl;
    cout << "  csvutil -t extract -p a.csv -n 0,2" << endl;
    cout << "  csvutil -t filter -p a.csv -f a -c '>3'" << endl;
    cout << "  csvutil -t filter -p a.csv -f a -c !=b" << endl;
    cout << "  csvutil -t filter -p a.csv -f a -c %2=1" << endl;
}

std::vector<std::string> split(const std::string& str, char delimiter_);
void print_selected(std::vector<std::string>& datum, std::vector<int> selected);
bool read_file();
bool parse_header();
void handle_sort();
void handle_extract();
void handle_filter();
int get_field_idx(const string& name);

int main(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hxyp:t:f:n:c:d:")) != -1) {
        switch (opt) {
            case 'x':
                has_header = false;
                break;
            case 'y':
                sort_numerically = false;
                break;
            case 'd':
                delimiter = *optarg;
                break;
            case 'p':
                path = std::string(optarg);
                break;
            case 't':
                type = std::string(optarg);
                break;
            case 'f':
                field_names = split(optarg, ',');
                break;
            case 'n':
                field_names = split(optarg, ',');
                for (auto& name : field_names) {
                    field_idx.push_back(std::stoi(name));
                }
                field_names.clear();
                break;
            case 'c':
                filter_condition = std::string(optarg);
                break;
            case 'h':
            default:
                help();
                return 0;
        }
    }

    if (!read_file() || contents.empty()) {
        cerr << "file read failed or empty file " << path << endl;
        return 1;
    }
    if (!parse_header()) return 1;

    if (type == "sort") {
        handle_sort();
    } else if (type == "extract") {
        handle_extract();
    } else if (type == "filter") {
        handle_filter();
    } else {
        cerr << "unknown type " << type << endl;
    }
}

enum CmpType { Unknown, Great, Less, GreatEqual, LessEqual, InEqual, LineModulus };

void handle_filter() {
    if (field_idx.empty() || filter_condition.empty()) {
        cerr << "filter field or condition not specified" << endl;
        return;
    }
    size_t left_field_idx = (std::size_t)field_idx.front();
    if (!header_columns.empty() && left_field_idx >= header_columns.size()) {
        cerr << "filter field out of bound " << left_field_idx << " >= " << header_columns.size() << endl;
        return;
    }
    CmpType cmpType = CmpType::Unknown;
    string operand;
    int modulus1 = 0, modulus2 = 0;
    if (filter_condition.substr(0, 2) == ">=") {
        cmpType = CmpType::GreatEqual;
        operand = filter_condition.substr(2);
    } else if (filter_condition.substr(0, 2) == "<=") {
        cmpType = CmpType::LessEqual;
        operand = filter_condition.substr(2);
    } else if (filter_condition.substr(0, 2) == "!=") {
        cmpType = CmpType::InEqual;
        operand = filter_condition.substr(2);
    } else if (filter_condition.substr(0, 1) == ">") {
        cmpType = CmpType::Great;
        operand = filter_condition.substr(1);
    } else if (filter_condition.substr(0, 1) == "<") {
        cmpType = CmpType::Less;
        operand = filter_condition.substr(1);
    } else if (filter_condition.substr(0, 1) == "%") {
        cmpType = CmpType::LineModulus;
        auto tmp_lets = ztool::split(filter_condition.substr(1), '=');
        modulus1 = std::stoi(tmp_lets.front());
        modulus2 = std::stoi(tmp_lets.back());
    }

    if (cmpType == CmpType::Unknown) {
        cerr << "unknown filter direction " << filter_condition[0] << endl;
        return;
    }

    bool cmp_rhs_value = true;
    int operand_idx = -1;
    double filter_val = 0;
    if (std::isalpha(operand[0])) {
        cmp_rhs_value = false;
        operand_idx = get_field_idx(operand);
        if (operand_idx < 0) return;
    } else {
        filter_val = std::stod(filter_condition.substr(1));
    }

    cout << head_line << endl;
    int line_number = 0;
    for (const auto& line : contents) {
        ++line_number;
        vector<string> lets = split(line, delimiter);
        double val = std::stod(lets[left_field_idx]);
        double operand_val = filter_val;
        if (!cmp_rhs_value) {
            operand_val = std::stod(lets[operand_idx]);
        }
        if ((cmpType == CmpType::Great && val > operand_val) || (cmpType == CmpType::Less && val < operand_val) ||
            (cmpType == CmpType::GreatEqual && val >= operand_val) ||
            (cmpType == CmpType::LessEqual && val <= operand_val) ||
            (cmpType == CmpType::InEqual && val != operand_val) ||
            (cmpType == CmpType::LineModulus && line_number % modulus1 == modulus2))
            cout << line << endl;
    }
}

void handle_extract() {
    if (field_idx.empty()) {
        cerr << "extract field not specified" << endl;
        return;
    }

    print_selected(header_columns, field_idx);
    for (const auto& line : contents) {
        vector<string> lets = split(line, delimiter);
        cout << "\n";
        print_selected(lets, field_idx);
    }
}

struct Entry {
    string line;
    double val;
    string val_str;
};

bool cmp_numeric(const Entry& l, const Entry& r) {
    if (std::isfinite(l.val) && std::isfinite(r.val))
        return l.val < r.val;
    else if (!std::isfinite(l.val) && !std::isfinite(r.val))
        return false;  // 永远让比较函数对相同元素返回false, otherwise violite Strict Weak Ordering
    else
        return !std::isfinite(l.val);
}

bool cmp_string(const Entry& l, const Entry& r) { return l.val_str < r.val_str; }

void handle_sort() {
    if (field_idx.empty()) {
        cerr << "sort field not specified" << endl;
        return;
    }
    size_t sort_field_idx = (size_t)field_idx.front();
    if (!header_columns.empty() && sort_field_idx >= header_columns.size()) {
        cerr << "sort field out of bound " << sort_field_idx << " >= " << header_columns.size() << endl;
        return;
    }

    vector<Entry> datum;
    Entry entry;
    for (const auto& line : contents) {
        vector<string> lets = split(line, delimiter);
        entry.line = line;
        entry.val_str = lets[sort_field_idx];
        if (sort_numerically) {
            entry.val = std::stod(entry.val_str);
        }
        datum.push_back(entry);
    }

    if (sort_numerically)
        std::sort(datum.begin(), datum.end(), cmp_numeric);
    else
        std::sort(datum.begin(), datum.end(), cmp_string);

    cout << head_line << endl;
    for (const auto& entry_ : datum) cout << entry_.line << endl;
}

int get_field_idx(const string& name) {
    auto itr = std::find(header_columns.begin(), header_columns.end(), name);
    if (itr == header_columns.end()) {
        cerr << "column " << name << " not found!" << endl;
        return -1;
    }
    return itr - header_columns.begin();
}

bool parse_header() {
    if (!has_header) return true;
    head_line = contents.front();
    contents.erase(contents.begin());  // remove head line
    header_columns = split(head_line, delimiter);
    if (!field_names.empty()) {
        for (const auto& name : field_names) {
            int tmp_idx = get_field_idx(name);
            if (tmp_idx < 0) return false;
            field_idx.push_back(tmp_idx);
        }
    }
    return true;
}

bool read_file() {
    ifstream ifs(path, ifstream::in);

    if (ifs.is_open()) {
        string s;
        while (getline(ifs, s)) {
            if (!s.empty()) contents.push_back(s);
        }
        ifs.close();
        return true;
    } else {
        return false;
    }
}

void print_selected(std::vector<std::string>& datum, std::vector<int> selected) {
    int total_column = (int)selected.size();
    for (int i = 0; i < total_column - 1; ++i) {
        cout << datum[selected[i]] << ",";
    }
    cout << datum[selected.back()];
}

std::vector<std::string> split(const std::string& str, char delimiter_) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter_);
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter_, start);
    }
    result.push_back(str.substr(start, end));
    return result;
}
