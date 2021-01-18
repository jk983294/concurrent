#include <unistd.h>
#include <zerg_admin.h>
#include <iomanip>

using namespace std;
using namespace ztool;

string key;

void help() {
    std::cout << "Program options:" << std::endl;
    std::cout << "  -h                                    list help" << std::endl;
    std::cout << "  -k                                    key" << std::endl;
}

int main(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hk:")) != -1) {
        switch (opt) {
            case 'k':
                key = std::string(optarg);
                break;
            case 'h':
            default:
                help();
                return 1;
        }
    }

    if (key.empty()) {
        help();
        return 1;
    }

    auto* admin = new Admin(key);
    admin->OpenForCreate();
    while (1) {
        string cmd = admin->ReadCmd();
        if (!cmd.empty()) {
            cout << "recv " << cmd << endl;
            admin->WriteReturn(cmd);
        }

        sleep(1);
    }
}
