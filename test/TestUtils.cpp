#include <utils/Utils.h>
#include "catch.hpp"

using namespace frenzy;

TEST_CASE("utils functions", "[utils]") {
    REQUIRE(nextPowerOf2(1) == 1);
    REQUIRE(nextPowerOf2(2) == 2);
    REQUIRE(nextPowerOf2(3) == 4);
    REQUIRE(nextPowerOf2(5) == 8);
    REQUIRE(nextPowerOf2(16) == 16);
    REQUIRE(nextPowerOf2(17) == 32);
}
