#include <boost/circular_buffer.hpp>
#include "catch.hpp"

using namespace std;

TEST_CASE("circular buffer", "[circular buffer]") {
    boost::circular_buffer<int> cbuf(3);

    cbuf.push_back(1);
    cbuf.push_back(2);
    REQUIRE(cbuf.size() == 2);

    cbuf.push_back(1);
    cbuf.push_back(2);
    cbuf.push_back(3);
    cbuf.push_back(4);
    cbuf.push_back(5);
    cbuf.push_back(6);
    REQUIRE(cbuf.size() == 3);

    cbuf.push_back(1);
    cbuf.push_back(2);
    cbuf.push_back(3);
    cbuf.push_back(4);
    cbuf.push_back(5);
    cbuf.push_back(6);
    REQUIRE(cbuf.front() == 4);

    cbuf.push_back(1);
    cbuf.push_back(2);
    cbuf.push_back(3);
    cbuf.push_back(4);
    cbuf.push_back(5);
    cbuf.push_back(6);
    cbuf.pop_front();
    REQUIRE(cbuf.front() == 5);

    cbuf.push_back(1);
    cbuf.push_back(2);
    cbuf.push_back(3);
    cbuf.push_back(4);
    cbuf.push_back(5);
    cbuf.push_back(6);
    cbuf.pop_front();
    REQUIRE(cbuf.size() == 2);
}
