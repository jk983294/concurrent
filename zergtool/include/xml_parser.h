#ifndef _ZERG_XML_PARSER_H_
#define _ZERG_XML_PARSER_H_

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "zerg_exception.h"

namespace ztool {

class XmlNode {
private:
    XmlNode* root{nullptr};
    bool is_root{false};
    rapidxml::xml_node<>* p_root{nullptr};
    rapidxml::xml_node<>* p_node{nullptr};
    std::vector<std::shared_ptr<rapidxml::xml_document<>>> copied_docs;
    std::vector<std::shared_ptr<rapidxml::file<>>> copied_files;

    std::shared_ptr<rapidxml::xml_document<>> doc;
    std::shared_ptr<rapidxml::file<>> file;

public:
    XmlNode() = default;

    XmlNode(const XmlNode& node) {
        p_node = node.p_node;
        is_root = node.is_root;
        p_root = node.p_root;
        root = node.root;
        doc = node.doc;
    }

    XmlNode(rapidxml::xml_node<>* node, rapidxml::xml_node<>* rapid_root,
            const std::shared_ptr<rapidxml::xml_document<>>& ndoc, const std::shared_ptr<rapidxml::file<>>& nfile) {
        if (node) {
            p_node = node;
            p_root = rapid_root;
            doc = ndoc;
            file = nfile;
            root = this;
        } else
            THROW_ZERG_EXCEPTION("xmlparser cannot get node");
        if (rapid_root == node) is_root = true;
    }

    XmlNode(rapidxml::xml_node<>* node, rapidxml::xml_node<>* rapid_root,
            const std::shared_ptr<rapidxml::xml_document<>>& ndoc, const std::shared_ptr<rapidxml::file<>>& nfile,
            XmlNode* rt) {
        if (node) {
            p_node = node;
            p_root = rapid_root;
            doc = ndoc;
            file = nfile;
            root = rt;
        } else
            THROW_ZERG_EXCEPTION("xml parser cannot get node");
        if (rapid_root == node) is_root = true;
    }

    //! load a root node (document) from stream
    explicit XmlNode(std::istream& in) { load(in); }

    /// load a xml file from stream
    void load(std::istream& in) {
        file = std::make_shared<rapidxml::file<>>(in);
        doc = std::make_shared<rapidxml::xml_document<>>();
        doc->parse<0>(file->data());
        p_node = doc->first_node();
        p_root = p_node;
        is_root = true;
        root = this;
    }

    //! load a root node (document) from xml file
    explicit XmlNode(std::string fileName) { load(fileName); }

    void load(const std::string& fileName) {
        std::ifstream theFile(fileName);
        if (!theFile.is_open()) THROW_ZERG_EXCEPTION("cannot open xml file: " << fileName);

        theFile.seekg(0, std::ios::end);
        if (theFile.tellg() == 0) {
            THROW_ZERG_EXCEPTION("xml file is empty: " << fileName);
        }

        file = std::make_shared<rapidxml::file<>>(fileName.c_str());
        doc = std::make_shared<rapidxml::xml_document<>>();
        doc->parse<0>(file->data());
        p_node = doc->first_node();
        p_root = p_node;
        root = this;
        is_root = true;
    }

    //! assignment
    XmlNode& operator=(const XmlNode& other) {
        if (this != &other) {  // protect against invalid self-assignment
            // allocate new memory and copy the elements
            p_node = other.p_node;
            is_root = other.is_root;
            p_root = other.p_root;
            doc = other.doc;
            root = other.root;
            file = other.file;
        }
        return *this;
    }

    //! get root node
    XmlNode getRoot() const { return XmlNode(p_root, p_root, doc, file); };

    //! get parent node, return nullptr if this is root
    XmlNode getParent() const {
        if (is_root) {
            THROW_ZERG_EXCEPTION("failed to get parent of root node");
        } else
            return XmlNode(p_node->parent(), p_root, doc, file);
    }

    //! whether the node has a child
    bool hasChild(const std::string& childid) const {
        bool is_child_exist;
        if (childid.empty())
            is_child_exist = p_node->first_node();
        else
            is_child_exist = p_node->first_node(childid.c_str());

        return is_child_exist;
    }

    //! get the first child node
    XmlNode getChild(const std::string& childid = "") const {
        if (!hasChild(childid)) THROW_ZERG_EXCEPTION("failed to get child:" << childid);

        if (childid.empty())
            return XmlNode(p_node->first_node(), p_root, doc, file, root);
        else
            return XmlNode(p_node->first_node(childid.c_str()), p_root, doc, file, root);
    }

    //! get all children, matching node name
    std::vector<XmlNode> getChildren(const std::string& nodeName) const {
        std::vector<XmlNode> children;
        auto child = p_node->first_node(nodeName.c_str());
        while (child) {
            children.emplace_back(child, p_root, doc, file, root);
            child = child->next_sibling(nodeName.c_str());
        }
        return children;
    }

    //! get all children
    std::vector<XmlNode> getChildren() const {
        std::vector<XmlNode> children;
        auto child = p_node->first_node();
        while (child) {
            children.emplace_back(child, p_root, doc, file, root);
            child = child->next_sibling();
        }
        return children;
    }

    //! get all attributes: key is attribute name, value is attribute value
    std::unordered_map<std::string, std::string> getAllAttr() const {
        std::unordered_map<std::string, std::string> attributes;
        auto attr = p_node->first_attribute();
        while (attr) {
            attributes[attr->name()] = attr->value();
            attr = attr->next_attribute();
        }
        return attributes;
    }

    //! get all attributes: key is attribute name, value is attribute value
    std::map<std::string, std::string> getAllAttrMap() const {
        std::map<std::string, std::string> attributes;
        auto attr = p_node->first_attribute();
        while (attr) {
            attributes[attr->name()] = attr->value();
            attr = attr->next_attribute();
        }
        return attributes;
    }

    bool hasAttr(const std::string& key) const {
        auto attr = p_node->first_attribute(key.c_str());
        if (attr)
            return true;
        else
            return false;
    }

    //! get a string. default is ""
    std::string getAttrOrThrow(const std::string& key) const {
        auto attr = p_node->first_attribute(key.c_str());
        if (attr)
            return std::string(attr->value());
        else
            THROW_ZERG_EXCEPTION("getAttrOrThrow cannot find key=" << key);
    }

    //! get a string. default is ""
    std::string getAttrString(const std::string& key) const {
        auto attr = p_node->first_attribute(key.c_str());
        if (attr)
            return std::string(attr->value());
        else
            return "";
    }

    std::string print() const {
        std::stringstream ss;
        ss << *p_node;
        return ss.str();
    }

    //! get a string 1
    std::string getAttrDefault(const std::string& key, const char* defaultValue) const {
        auto attr = p_node->first_attribute(key.c_str());
        std::string ans, ret;
        if (attr)
            ans = std::string(attr->value());
        else
            return defaultValue;
        return ans;
    }

    //! get a string 2
    std::string getAttrDefault(const std::string& key, const std::string& defaultValue) const {
        return getAttrDefault(key, defaultValue.c_str());
    }

    //! get a string 3
    std::string getAttrDefault(const std::string& key, char* defaultValue) const {
        return getAttrDefault(key, const_cast<const char*>(defaultValue));
    }

    //! get an integer
    int getAttrDefault(const std::string& key, int defaultValue) const {
        auto attr = p_node->first_attribute(key.c_str());
        std::string ret;
        if (attr)
            ret = std::string(attr->value());
        else
            return defaultValue;
        return std::stoi(ret);
    }

    long getAttrDefault(const std::string& key, long defaultValue) const {
        auto attr = p_node->first_attribute(key.c_str());
        std::string ret;
        if (attr)
            ret = std::string(attr->value());
        else
            return defaultValue;
        return std::stol(ret);
    }

    size_t getAttrDefault(const std::string& key, size_t defaultValue) const {
        auto attr = p_node->first_attribute(key.c_str());
        std::string ret;
        if (attr)
            ret = std::string(attr->value());
        else
            return defaultValue;
        return std::stoul(ret);
    }

    //! get a bool
    bool getAttrDefault(const std::string& key, bool defaultValue) const {
        auto attr = p_node->first_attribute(key.c_str());
        std::string ans, ret;
        if (!attr) {
            return defaultValue;
        }
        if (std::string(attr->value()) == "true")
            return true;
        else if (std::string(attr->value()) == "false")
            return false;
        else
            return defaultValue;
    }

    //! get a double
    double getAttrDefault(const std::string& key, double defaultValue) const {
        auto attr = p_node->first_attribute(key.c_str());
        std::string ret;
        if (attr)
            ret = std::string(attr->value());
        else
            return defaultValue;
        return std::stod(ret);
    }

    float getAttrDefault(const std::string& key, float defaultValue) const {
        auto attr = p_node->first_attribute(key.c_str());
        std::string ret;
        if (attr)
            ret = std::string(attr->value());
        else
            return defaultValue;
        return std::stof(ret);
    }

    //! get node name
    std::string getNodeName() const { return p_node->name(); }
    //! is root node
    bool isRoot() const { return is_root; }

    //! append a child node to the current node. return appended child
    XmlNode appendChild(const std::string& nodeName) {
        char* node_name = doc->allocate_string(nodeName.c_str());
        auto node = doc->allocate_node(rapidxml::node_element, node_name);
        p_node->append_node(node);
        return XmlNode(node, p_root, doc, file, root);
    }

    XmlNode appendChild(XmlNode& new_node) {
        auto node = doc->clone_node(new_node.p_node);
        p_node->append_node(node);
        copied_docs.push_back(new_node.doc);
        copied_files.push_back(new_node.file);
        return XmlNode(node, p_root, doc, file, root);
    }

    XmlNode appendChild(XmlNode& new_node, XmlNode& rt) {
        auto node = doc->clone_node(new_node.p_node);
        p_node->append_node(node);
        rt.copied_docs.push_back(new_node.doc);
        rt.copied_files.push_back(new_node.file);
        return XmlNode(node, p_root, doc, file, &rt);
    }

    //! append attribute to current node
    void appendAttr(const std::string& name, const char* value) {
        auto attr = p_node->first_attribute(name.c_str());
        char* new_value = doc->allocate_string(value);
        if (attr) {
            attr->value(new_value);
        } else {
            char* new_name = doc->allocate_string(name.c_str());
            attr = doc->allocate_attribute(new_name, new_value);
            p_node->append_attribute(attr);
        }
    }

    void appendAttr(const std::string& name, const std::string& value) { return appendAttr(name, value.c_str()); }
    void appendAttr(const std::string& name, float value) {
        return appendAttr(name, std::to_string((long double)value));
    }

    void appendAttr(const std::string& name, int value) { return appendAttr(name, std::to_string((long long)value)); }

    void appendAttr(const std::string& name, bool value) {
        if (value)
            appendAttr(name, "true");
        else
            appendAttr(name, "false");
    }
};

}  // namespace ztool
#endif
