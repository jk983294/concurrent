#include "ZBase64.h"
#include "catch.hpp"

TEST_CASE("base64", "[base64]") {
    REQUIRE(Base64Encode("my test message") == std::string{"bXkgdGVzdCBtZXNzYWdl"});
    REQUIRE(Base64Decode("bXkgdGVzdCBtZXNzYWdl") == std::string{"my test message"});
}
