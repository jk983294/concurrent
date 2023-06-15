#include "catch.hpp"
#include "zerg_conversion.h"
#include "zerg_time.h"
#include "zerg_util.h"

using namespace std;
using namespace ztool;

TEST_CASE("GenAllDaysInYear", "[time]") {
    std::vector<long> ret = ztool::GenAllDaysInYear(2018);
    REQUIRE(ret.front() == 20180101);
    REQUIRE(ret.back() == 20181231);
}

TEST_CASE("conversion", "[time]") {
    REQUIRE(ztool::fast_atoi("12") == 12);
    REQUIRE(ztool::fast_atoi("-12") == -12);
    REQUIRE(ztool::fast_atoi("012") == 12);
    REQUIRE(ztool::fast_atoi("-012") == -12);

    ztool::ZergTime time = ztool::StringToTime("08:42:21");
    REQUIRE(time.hour == 8);
    REQUIRE(time.minute == 42);
    REQUIRE(time.second == 21);
}

TEST_CASE("Constructor", "[time]") {
    auto clock = new Clock<>();
    clock->Update();
    REQUIRE(clock->DateToInt() >= 20180718);

    auto clock1 = new Clock<>("19:20:20", "%H:%M:%S");
    REQUIRE(clock1->TimeToInt() == 192020000);
}

TEST_CASE("Comparison", "[time]") {
    Clock<> clock1("19:20:20", "%H:%M:%S");
    Clock<> clock2("19:20:21", "%H:%M:%S");
    REQUIRE(clock2 > clock1);
    REQUIRE(clock2 >= clock1);

    Clock<> clock3("19:20:20", "%H:%M:%S");
    REQUIRE(clock3 == clock1);
}

TEST_CASE("Time +", "[time]") {
    Clock<> clock1("19:20:50", "%H:%M:%S");
    Clock<> clock3("19:20:51", "%H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 + ztool::zergtime::millisecond * 500;
    clock2 = clock2 + ztool::zergtime::millisecond * 500;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time +=", "[time]") {
    Clock<> clock1("19:20:50", "%H:%M:%S");
    Clock<> clock2("19:20:51", "%H:%M:%S");
    clock1 += ztool::zergtime::millisecond * 500;
    clock1 += ztool::zergtime::millisecond * 500;
    REQUIRE(clock1 == clock2);
}

TEST_CASE("Time + second", "[time]") {
    Clock<> clock1("19:20:50", "%H:%M:%S");
    Clock<> clock3("19:20:55", "%H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 + ztool::zergtime::second * 5;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time + minute", "[time]") {
    Clock<> clock1("19:19:20", "%H:%M:%S");
    Clock<> clock3("19:21:20", "%H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 + ztool::zergtime::minute * 2;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time + hour", "[time]") {
    Clock<> clock1("18:20:20", "%H:%M:%S");
    Clock<> clock3("19:20:20", "%H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 + ztool::zergtime::hour * 1;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time + day", "[time]") {
    Clock<> clock1("20140601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock3("20140602 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 + ztool::zergtime::day * 1;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time + week", "[time]") {
    Clock<> clock1("20140601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock3("20140608 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 + ztool::zergtime::week * 1;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time + year", "[time]") {
    Clock<> clock1("20140601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock3("20150601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 + ztool::zergtime::year * 1;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time - ms", "[time]") {
    Clock<> clock1("20150601 19:20:21", "%Y%m%d %H:%M:%S");
    Clock<> clock3("20150601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 - ztool::zergtime::millisecond * 500;
    clock2 = clock2 - ztool::zergtime::millisecond * 500;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time -= ms", "[time]") {
    Clock<> clock1("20150601 19:20:21", "%Y%m%d %H:%M:%S");
    Clock<> clock3("20150601 19:20:20", "%Y%m%d %H:%M:%S");
    clock1 -= ztool::zergtime::millisecond * 500;
    clock1 -= ztool::zergtime::millisecond * 500;
    REQUIRE(clock1 == clock3);
}

TEST_CASE("Time - hour", "[time]") {
    Clock<> clock1("20140601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock3("20140601 17:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 - ztool::zergtime::hour * 2;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time - year", "[time]") {
    Clock<> clock1("20150601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock3("20140601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 - ztool::zergtime::year * 1;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time chain", "[time]") {
    Clock<> clock1("20140601 19:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock3("20150601 17:20:20", "%Y%m%d %H:%M:%S");
    Clock<> clock2;
    clock2 = clock1 - ztool::zergtime::hour * 2 + ztool::zergtime::year;
    REQUIRE(clock2 == clock3);
}

TEST_CASE("Time trading clock", "[time]") {
    vector<long> list = {20140530, 20140602, 20140603};
    Clock<TimePolicy::TradingDay> clock("20140601 19:20:20", "%Y%m%d %H:%M:%S", list);
    REQUIRE(clock.DateToInt() == 20140602);
}

TEST_CASE("Time trading clock late than 18:00", "[time]") {
    vector<long> list = {20140530, 20140602, 20140603};
    Clock<TimePolicy::TradingDay> clock("20140602 19:20:20", "%Y%m%d %H:%M:%S", list);
    REQUIRE(clock.DateToInt() == 20140603);
}

TEST_CASE("Time trading clock late than 24:00", "[time]") {
    vector<long> list = {20140530, 20140602, 20140603};
    Clock<TimePolicy::TradingDay> clockT("20140602 2:20:20", "%Y%m%d %H:%M:%S", list);
    REQUIRE(clockT.TimeToInt() == 262020000);
    REQUIRE(clockT.DateToInt() == 20140602);
}

TEST_CASE("Human Readable", "[time]") {
    auto ret = ztool::HumanReadableMillisecond("100ms");
    REQUIRE(ret == 100);

    ret = ztool::HumanReadableMillisecond("100milliseconds");
    REQUIRE(ret == 100);

    ret = ztool::HumanReadableMillisecond("100millisecond");
    REQUIRE(ret == 100);

    ret = ztool::HumanReadableMillisecond(" 10 min ");
    REQUIRE(ret == 600000);

    ret = ztool::HumanReadableMillisecond(" 10 minutes ");
    REQUIRE(ret == 600000);

    ret = ztool::HumanReadableMillisecond(" 10 minute ");
    REQUIRE(ret == 600000);

    ret = ztool::HumanReadableMicrosecond(" 10 us ");
    REQUIRE(ret == 10);

    ret = ztool::HumanReadableMicrosecond(" 10 macroseconds ");
    REQUIRE(ret == 10);

    ret = ztool::HumanReadableMicrosecond(" 10 macrosecond ");
    REQUIRE(ret == 10);

    ret = ztool::HumanReadableMicrosecond(" 10 sec ");
    REQUIRE(ret == 10 * 1000 * 1000);

    ret = ztool::HumanReadableMicrosecond(" 10 seconds ");
    REQUIRE(ret == 10 * 1000 * 1000);

    ret = ztool::HumanReadableMicrosecond(" 10 second ");
    REQUIRE(ret == 10 * 1000 * 1000);

    ret = ztool::HumanReadableMicrosecond(" 10 s ");
    REQUIRE(ret == 10 * 1000 * 1000);
}

TEST_CASE("Human Readable time", "[time]") {
    Clock<> clock1("21:01:12", "%H:%M:%S");
    std::string a(" 09:01:12 pm ");
    auto ret1 = ztool::HumanReadableTime(a);
    REQUIRE(ret1 == clock1);

    Clock<> clock2("09:01:12", "%H:%M:%S");
    std::string b(" 09:01:12 A.M.");
    auto ret2 = ztool::HumanReadableTime(b);
    REQUIRE(ret2 == clock2);

    Clock<> clock3("09:01:12", "%H:%M:%S");
    std::string c(" 09:01:12");
    auto ret3 = ztool::HumanReadableTime(c);
    REQUIRE(ret3 == clock3);

    Clock<> clock4("09:01", "%H:%M");
    std::string d(" 09:01");
    auto ret4 = ztool::HumanReadableTime(d);
    REQUIRE(ret4 == clock4);

    Clock<> clock5("21:01", "%H:%M");
    std::string e(" 09:01 PM");
    auto ret5 = ztool::HumanReadableTime(e);
    REQUIRE(ret5 == clock5);
}

TEST_CASE("GetNextDate", "[GetNextDate]") {
    REQUIRE(GetNextDate(20220929) == 20220930);
    REQUIRE(GetNextDate(20220930) == 20221001);
}
