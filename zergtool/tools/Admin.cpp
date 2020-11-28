#include <unistd.h>
#include <zerg_admin.h>
#include <iomanip>
#include <linenoise.hpp>

using namespace std;
using namespace ztool;

string key, cmd;
bool continous{false};

void help() {
    std::cout << "Program options:" << std::endl;
    std::cout << "  -h                                    list help" << std::endl;
    std::cout << "  -k                                    key" << std::endl;
    std::cout << "  -c                                    cmd" << std::endl;
    std::cout << "  -x                                    continous mode" << std::endl;
}

int main(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hxk:c:")) != -1) {
        switch (opt) {
            case 'k':
                key = std::string(optarg);
                break;
            case 'c':
                cmd = std::string(optarg);
                break;
            case 'x':
                continous = true;
                break;
            case 'h':
            default:
                help();
                return 1;
        }
    }

    if (key.empty() || (!continous && cmd.empty())) {
        help();
        return 1;
    }

    auto* admin = new Admin(key);
    admin->OpenForRead();
    if (!admin->shm_status) {
        cerr << "Admin failed for key " << key << endl;
        return -1;
    }

    if (continous) {
        linenoise::SetMultiLine(false);
        linenoise::SetHistoryMaxLen(100);
        while (true) {
            std::string line;

            auto quit = linenoise::Readline("> ", line);

            if (quit) {
                break;
            }

            admin->IssueCmd(line);

            linenoise::AddHistory(line.c_str());
        }
    } else {
        admin->IssueCmd(cmd);
    }
}
