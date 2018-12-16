#include <container/AtomicHashMap.h>
#include "catch.hpp"

using namespace frenzy;

TEST_CASE("AtomicHashMap insert", "[AtomicHashMap]") {
    AtomicHashMap<std::string, std::string> m(100);

    m.emplace("abc", "ABC");
    REQUIRE(m.find("abc") != m.cend());
    REQUIRE(m.find("abc")->first == "abc");
    REQUIRE(m.find("abc")->second == "ABC");
    REQUIRE(m.find("def") == m.cend());
    auto iter = m.cbegin();
    REQUIRE(iter != m.cend());
    REQUIRE(iter == m.find("abc"));
    auto a = iter;
    REQUIRE(a == iter);
    auto b = iter;
    ++iter;
    REQUIRE(iter == m.cend());
    REQUIRE(a == b);
    REQUIRE(a != iter);
    a++;
    REQUIRE(a == iter);
    REQUIRE(a != b);
}

TEST_CASE("AtomicHashMap load factor", "[AtomicHashMap]") {
    AtomicHashMap<int, bool> m(5000, 0.5f);
    // we should be able to put in much more than 5000 things because of our load factor request
    for (int i = 0; i < 10000; ++i) {
        m.emplace(i, true);
    }
}

TEST_CASE("AtomicHashMap load factor exceed", "[AtomicHashMap]") {
    AtomicHashMap<int, bool> m(5000, 1.0f);
    REQUIRE_THROWS([&]() {
        // we should be able to put in much more than 5000 things because of our load factor request
        for (int i = 0; i < 6000; ++i) {
            m.emplace(i, true);
        }
    }());
}

TEST_CASE("AtomicHashMap atomic value change", "[AtomicHashMap]") {
    AtomicHashMap<int, MutableAtom<int>> m(100, 0.6f);
    for (int i = 0; i < 50; ++i) {
        m.emplace(i, i);
    }

    m.find(1)->second.data++;
    REQUIRE(m.find(1)->second.data.load() == 2);
}

TEST_CASE("AtomicHashMap non atomic value change", "[AtomicHashMap]") {
    AtomicHashMap<int, MutableData<int>, NonAtomic> m(100, 0.6f);
    for (int i = 0; i < 50; ++i) {
        m.emplace(i, i);
    }

    m.find(1)->second.data++;
    REQUIRE(m.find(1)->second.data == 2);
}
