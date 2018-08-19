#include <log/ContextSaver.h>
#include "catch.hpp"

using namespace std;
using namespace frenzy;

uint8_t* buffer = new uint8_t[512];

template<typename T>
void test(T t) {
    ContextSaver saver;
    saver.save(t);
    T result{};
    saver.load(result);
    REQUIRE((result == t));
}

TEST_CASE("normal", "[context saver]") {
    char var0 = 'a';
    int var1 = 42;
    long var2 = 42;
    float var3 = 4.2;
    double var4 = 4.2;

    test(var0);
    test(var1);
    test(var2);
    test(var3);
    test(var4);
}

TEST_CASE("char array", "[context saver]") {
    ContextSaver saver;
    const char* str = "hello world.";
    saver.save(str);
    char resultBuffer[256];
    saver.load(resultBuffer);
    REQUIRE(std::strcmp(str, resultBuffer) == 0);
}

TEST_CASE("sequence", "[context saver]") {
    int var1 = 42;
    long var2 = 42;
    const char* var3 = "world.";
    float var4 = 4.2;
    double var5 = 4.2;

    ContextSaver saver;
    saver.save(var1);
    saver.save(var2);
    saver.save(var3);
    saver.save(var4);
    saver.save(var5);

    int result1 = 0;
    long result2 = 0;
    char result3[256];
    float result4 = 0;
    double result5 = 0;

    saver.load(result1);
    saver.load(result2);
    saver.load(result3);
    saver.load(result4);
    saver.load(result5);

    REQUIRE(var1 == result1);
    REQUIRE(var2 == result2);
    REQUIRE(std::strcmp(var3, result3) == 0);
    REQUIRE(var4 == result4);
    REQUIRE(var5 == result5);
}

TEST_CASE("sequence chain", "[context saver]") {
    int var1 = 42;
    long var2 = 42;
    const char* var3 = "world.";
    float var4 = 4.2;
    double var5 = 4.2;

    ContextSaver saver;
    saver.save(var1).save(var2).save(var3).save(var4).save(var5);

    int result1 = 0;
    long result2 = 0;
    char result3[256];
    float result4 = 0;
    double result5 = 0;

    saver.load(result1).load(result2).load(result3).load(result4).load(result5);

    REQUIRE(var1 == result1);
    REQUIRE(var2 == result2);
    REQUIRE(std::strcmp(var3, result3) == 0);
    REQUIRE(var4 == result4);
    REQUIRE(var5 == result5);
}

TEST_CASE("schedule", "[context saver]") {
    int var1 = 42;
    long var2 = 42;
    char var3[16] {"hello"};
    float var4 = 4.2;
    double var5 = 4.2;
    string var6{" world."};

    ContextSaver saver;
    saver.schedule(var1).schedule(var2).schedule(var3, 16).schedule(var4).schedule(var5).schedule(var6);
    saver.save();

    int result1 = 0;
    long result2 = 0;
    char result3[256];
    float result4 = 0;
    double result5 = 0;
    string result6;

    saver.load(result1).load(result2).load(result3).load(result4).load(result5).load(result5).load(result6);

    REQUIRE(var1 == result1);
    REQUIRE(var2 == result2);
    REQUIRE(std::strcmp(var3, result3) == 0);
    REQUIRE(var4 == result4);
    REQUIRE(var5 == result5);
    REQUIRE(var6 == result6);
}