#include "catch.hpp"
#include "zerg_util.h"

TEST_CASE("FOREACH", "[for each]") {
    std::vector<int> vec = {1, 2, 3, 4};
    int sum = 0;
    FOR_EACH(i, vec) { sum += *i; }

    REQUIRE(sum == 10);

    sum = 0;
    FOR_EACH_R(i, vec) { sum += *i; }
    REQUIRE(sum == 10);

    sum = 0;
    size_t countNum = 0;
    FOR_EACH_ENUMERATE(count, i, vec) {
        sum += *i;
        countNum += count;
    }
    REQUIRE(countNum == 6);
}

TEST_CASE("FOREACH map", "[for each map]") {
    std::map<int, int> mp;
    mp[0] = 1;
    mp[1] = 2;
    mp[2] = 3;
    int k_total = 0;
    int v_total = 0;
    FOR_EACH_KV(k, v, mp) {
        k_total += k;
        v_total += v;
    }
    REQUIRE(k_total == 3);
    REQUIRE(v_total == 6);
}

TEST_CASE("FOR_EACH_RANGE", "[for each range]") {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    int sum = 0;
    FOR_EACH_RANGE(i, vec.begin() + 1, vec.end()) { sum += *i; }

    REQUIRE(sum == 14);
}

TEST_CASE("split instrument ID and market", "[utils]") {
    char str[] = "399001.sse";
    char *a{nullptr}, *b{nullptr};
    ztool::SplitInstrumentID(str, a, b);
    REQUIRE(strcmp(b, "sse") == 0);
}

TEST_CASE("replace time holder", "[utils]") {
    ztool::Clock<> clock;
    clock.Set(100);
    std::string str = "${YYYY}-${MM}-${DD}-${HHMMSSmmm}.xml";
    auto str2 = str;
    ztool::ReplaceSpecialTimeHolder(str2);
    std::cout << "default to current timestamp" << str2 << std::endl;
    ReplaceSpecialTimeHolder(str, clock);
    REQUIRE(str == "1970-01-01-080140000.xml");
}

TEST_CASE("replace time holder with string time", "[utils]") {
    ztool::Clock<> clock;
    clock.Set(100);
    std::string str = "${YYYY}-${MM}-${DD}-${HHMMSSmmm}.xml";
    ztool::ReplaceSpecialTimeHolder(str, "2018/02/03 08:23:40", "%Y/%m/%d %H:%M:%S");
    REQUIRE(str == "2018-02-03-082340000.xml");
}

TEST_CASE("replace string holder", "[utils]") {
    std::map<std::string, std::string> string_holders;
    string_holders["path"] = "aa/debug";
    string_holders["lib"] = "aa/lib";
    std::string str = "1-${path}-${lib}-1.xml";
    ztool::ReplaceStringHolder(str, string_holders);
    REQUIRE(str == "1-aa/debug-aa/lib-1.xml");
}
