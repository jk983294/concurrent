#include <string>
#include "catch.hpp"

TEST_CASE("dummy", "[dummy]") { REQUIRE(std::string{"dummy"} == std::string{"dummy"}); }
