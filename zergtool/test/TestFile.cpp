#include <string>
#include "catch.hpp"
#include "zerg_file.h"

using namespace ztool;

TEST_CASE("file", "[file]") {
    REQUIRE(ztool::IsFileExisted("/opt/version/latest/test/trade.xml"));
    REQUIRE(!ztool::IsFileExisted("/opt/version/latest/test/trade_non_exist.xml"));
    REQUIRE(path_join("/home", "jk") == "/home/jk");
    REQUIRE(path_join("", "jk") == "jk");
    REQUIRE(path_join("/home", "", "jk", "a.txt") == "/home/jk/a.txt");
    REQUIRE(IsFile("/opt/version/latest/test/empty.test.file"));
    REQUIRE(!IsFile("/opt/version/latest/test/empty.test.file.not.exist"));
    REQUIRE(IsDir("/opt/version/latest/test"));
    REQUIRE(!IsFile("/opt/version/latest/te"));
    REQUIRE(!IsDirWritable("/"));
    REQUIRE(IsDirReadable("/"));
    REQUIRE(Dirname("/opt/version/latest/test/empty.test.file") == "/opt/version/latest/test");
}
