#include <string>
#include "catch.hpp"
#include "zerg_set.h"

using namespace ztool;
using namespace std;

TEST_CASE("zerg set", "[set]") {
    ZergSet<std::string> s;
    s.insert("one");
    REQUIRE(s.size() == 1);
    REQUIRE(s.find("one") != s.end());

    s.insert("two");
    s.insert("two");
    REQUIRE(s.size() == 2);
}
