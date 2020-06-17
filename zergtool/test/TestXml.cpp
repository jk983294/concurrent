#include <string>
#include "catch.hpp"
#include "xml_parser.h"

using namespace zerg;
using namespace ztool;
using namespace std;

TEST_CASE("not exist file or empty file", "[xml]") {
    REQUIRE_THROWS([]() { XmlNode xmlNode("/opt/version/latest/test/trade_non_exist.xml"); }());

    REQUIRE_THROWS([]() { XmlNode xmlNode("/opt/version/latest/test/empty.test.file"); }());

    REQUIRE_NOTHROW([]() {
        std::ifstream in("/opt/version/latest/test/empty.test.file");
        XmlNode xmlNode(ifstream);
    }());
}

TEST_CASE("node select", "[xml]") {
    XmlNode xmlNode("/opt/version/latest/test/trade.xml");
    REQUIRE(xmlNode.getRoot().getNodeName() == "zsim");

    REQUIRE_THROWS([&]() { xmlNode.getChild("non_exist_node"); }());

    // get first node
    REQUIRE(xmlNode.getChild().getNodeName() == "modules");

    REQUIRE_THROWS([&]() { xmlNode.getChild("no_child").getChild(); }());

    // get child by name
    REQUIRE(xmlNode.getChild("alpha").getNodeName() == "alpha");

    auto node = xmlNode.getChild("modules");
    REQUIRE(node.getChildren().size() == 20);

    REQUIRE(node.getParent().getNodeName() == "zsim");

    REQUIRE_THROWS([&]() { node.getRoot().getParent(); }());

    REQUIRE(node.getChild().getAllAttr().size() == 3);
    REQUIRE(node.getChild().getAllAttrMap().size() == 3);

    REQUIRE(node.getChild().getAttrDefault("id", "ok") == "TradeStrategyDemo");
    REQUIRE(node.getChild().getAttrDefault("idd", "ok") == "ok");
    REQUIRE(node.getChild().getAttrDefault("em", "") == "");

    REQUIRE(node.getChild().getAttrDefault("trytime", 2) == 2);

    REQUIRE(!node.getChild().getAttrDefault("close_disappear", false));
    REQUIRE(node.getChild().getAttrDefault("close_disappear_no_exist", true));

    node.getChild().appendAttr("new", 34);
    REQUIRE(node.getChild().getAttrDefault("new", 31) == 34);

    node.getChild().appendAttr("new2", true);
    REQUIRE(node.getChild().getAttrDefault("new2", "empty") == "true");

    node.getChild().appendAttr("idd", "TradePortRose2");
    REQUIRE(node.getChild().getAttrDefault("idd", "ok") == "TradePortRose2");

    node.getChild().appendAttr("id", "TradePortRose2");
    REQUIRE(node.getChild().getAttrDefault("id", "ok") == "TradePortRose2");

    auto newNode = xmlNode.getChild("no_child");
    newNode.appendChild("new_node");
    REQUIRE(xmlNode.getChild("no_child").getChildren().size() == 1);
}
