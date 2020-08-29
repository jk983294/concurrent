#include "catch.hpp"
#include "zerg_string.h"

TEST_CASE("expand_names", "[expand_names]") {
    std::vector<string> ret = ztool::expand_names("f1,f2");
    std::vector<string> expected = {"f1", "f2"};
    REQUIRE(ret.size() == 2);
    REQUIRE(ret == expected);

    ret = ztool::expand_names("f[1-3],f2");
    expected = {"f1", "f2", "f3", "f2"};
    REQUIRE(ret == expected);

    ret = ztool::expand_names("f[1-3],f2,f[1-2]");
    expected = {"f1", "f2", "f3", "f2", "f1", "f2"};
    REQUIRE(ret == expected);

    ret = ztool::expand_names("f[1-");
    expected = {"f[1-"};
    REQUIRE(ret == expected);

    ret = ztool::expand_names("f[");
    expected = {"f["};
    REQUIRE(ret == expected);

    ret = ztool::expand_names("f]1[");
    expected = {"f]1["};
    REQUIRE(ret == expected);

    ret = ztool::expand_names("f]1[,");
    expected = {"f]1["};
    REQUIRE(ret == expected);
}
