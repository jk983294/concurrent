#include <csv.h>
#include <zerg_gz.h>
#include <ztool.h>

using namespace std;

string input_file;
string field;

void help() {
    std::cout << "Program options:" << std::endl;
    std::cout << "  -h                                    list help" << std::endl;
    std::cout << "  -p                                    input file" << std::endl;
    std::cout << "  -f                                    sort field" << std::endl;
    std::cout << "example:" << std::endl;
    std::cout << "csvsort -f my_column -p my.csv" << std::endl;
}

int main(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hp:f:")) != -1) {
        switch (opt) {
            case 'p':
                input_file = std::string(optarg);
                break;
            case 'h':
                help();
                return 0;
            case 'f':
                field = std::string(optarg);
                break;
            default:
                cerr << "unknown option" << endl;
                return 0;
        }
    }

    cout << "dummy tool " << input_file << " " << field << "\n";
    return 0;
}
