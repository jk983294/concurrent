#ifndef __CONFIGMANAGER_H_
#define __CONFIGMANAGER_H_

#include <folly_foreach.h>
#include <xml_parser.h>
#include <zerg_file.h>
#include <zerg_macro.h>
#include <cstddef>
#include <fstream>
#include <map>
#include <memory>
#include <vector>

template <typename T, typename U>
inline constexpr size_t offsetof_impl(T const* t, U T::*a) {
    return (char const*)t - (char const*)&(t->*a) >= 0 ? (char const*)t - (char const*)&(t->*a)
                                                       : (char const*)&(t->*a) - (char const*)t;
}

#define CLASS_OFFSET(Type_, Attr_) offsetof_impl((Type_ const*)nullptr, &Type_::Attr_)

#define CONFIG_EXAMPLE_XML(CLA)       \
    do {                              \
        _pConfigManager.eg_xml = CLA; \
    } while (0)

#define USE_CONFIGMANAGER static ztool::ConfigManager _pConfigManager;

#define CONFIG_SUMMARY(CLA)            \
    do {                               \
        _pConfigManager.eg_info = CLA; \
    } while (0)

#define ALLOW_PARSECONFIG(CLA)                 \
    extern "C" {                               \
    ztool::ConfigManager* GetConfigManager() { \
        auto* p = (CLA*)malloc(sizeof(CLA));   \
        _pConfigManager.data = (char*)p;       \
        CLA::INIT_PARAM();                     \
        _pConfigManager.PrintConfigs();        \
        free(p);                               \
        return &_pConfigManager;               \
    }                                          \
    }

#define REGISTER_CONFIG(p)               \
    do {                                 \
        _pConfigManager.ID = p->getID(); \
        _pConfigManager.data = (char*)p; \
    } while (0)

#define REQUIRE_CONFIG_FLOAT32(key, T, value_, rec, _info)                     \
    do {                                                                       \
        auto value = (float*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<float>(value, true, rec);      \
        newconfig->is_required = true;                                         \
        newconfig->magic_num = 1;                                              \
        newconfig->info = _info;                                               \
        _pConfigManager.configs[key] = newconfig;                              \
    } while (0)

#define REQUIRE_CONFIG_FLOAT64(key, T, value_, rec, _info)                      \
    do {                                                                        \
        auto value = (double*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<double>(value, true, rec);      \
        newconfig->is_required = true;                                          \
        newconfig->magic_num = 2;                                               \
        newconfig->info = _info;                                                \
        _pConfigManager.configs[key] = newconfig;                               \
    } while (0)

#define REQUIRE_CONFIG_INT32(key, T, value_, rec, _info)                         \
    do {                                                                         \
        auto value = (int32_t*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<int32_t>(value, true, rec);      \
        newconfig->is_required = true;                                           \
        newconfig->magic_num = 3;                                                \
        newconfig->info = _info;                                                 \
        _pConfigManager.configs[key] = newconfig;                                \
    } while (0)

#define REQUIRE_CONFIG_INT64(key, T, value_, rec, _info)                         \
    do {                                                                         \
        auto value = (int64_t*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<int64_t>(value, true, rec);      \
        newconfig->is_required = true;                                           \
        newconfig->magic_num = 4;                                                \
        newconfig->info = _info;                                                 \
        _pConfigManager.configs[key] = newconfig;                                \
    } while (0)

#define REQUIRE_CONFIG_BOOL(key, T, value_, rec, _info)                       \
    do {                                                                      \
        auto value = (bool*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<bool>(value, true, rec);      \
        newconfig->is_required = true;                                        \
        newconfig->magic_num = 5;                                             \
        _pConfigManager.configs[key] = newconfig;                             \
        newconfig->info = _info;                                              \
    } while (0)

#define REQUIRE_CONFIG_STRING(key, T, value_, rec, _info)                            \
    do {                                                                             \
        auto value = (std::string*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<std::string>(value, true, rec);      \
        newconfig->is_required = true;                                               \
        newconfig->magic_num = 6;                                                    \
        newconfig->info = _info;                                                     \
        _pConfigManager.configs[key] = newconfig;                                    \
    } while (0)

#define OPTIONAL_CONFIG_FLOAT32(key, T, value_, deft, _info)                   \
    do {                                                                       \
        auto value = (float*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<float>(value, false, deft);    \
        newconfig->is_required = false;                                        \
        newconfig->magic_num = 1;                                              \
        newconfig->info = _info;                                               \
        _pConfigManager.configs[key] = newconfig;                              \
    } while (0)

#define OPTIONAL_CONFIG_FLOAT64(key, T, value_, deft, _info)                    \
    do {                                                                        \
        auto value = (double*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<double>(value, false, deft);    \
        newconfig->is_required = false;                                         \
        newconfig->magic_num = 2;                                               \
        newconfig->info = _info;                                                \
        _pConfigManager.configs[key] = newconfig;                               \
    } while (0)

#define OPTIONAL_CONFIG_INT32(key, T, value_, deft, _info)                       \
    do {                                                                         \
        auto value = (int32_t*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<int>(value, false, deft);        \
        newconfig->is_required = false;                                          \
        newconfig->magic_num = 3;                                                \
        newconfig->info = _info;                                                 \
        _pConfigManager.configs[key] = newconfig;                                \
    } while (0)

#define OPTIONAL_CONFIG_INT64(key, T, value_, deft, _info)                       \
    do {                                                                         \
        auto value = (int64_t*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<long>(value, false, deft);       \
        newconfig->is_required = false;                                          \
        newconfig->magic_num = 4;                                                \
        newconfig->info = _info;                                                 \
        _pConfigManager.configs[key] = newconfig;                                \
    } while (0)

#define OPTIONAL_CONFIG_BOOL(key, T, value_, deft, _info)                     \
    do {                                                                      \
        auto value = (bool*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<bool>(value, false, deft);    \
        newconfig->is_required = false;                                       \
        newconfig->magic_num = 5;                                             \
        newconfig->info = _info;                                              \
        _pConfigManager.configs[key] = newconfig;                             \
    } while (0)

#define OPTIONAL_CONFIG_STRING(key, T, value_, deft, _info)                          \
    do {                                                                             \
        auto value = (std::string*)(_pConfigManager.data + CLASS_OFFSET(T, value_)); \
        auto newconfig = new ztool::ConfigValue<std::string>(value, false, deft);    \
        newconfig->is_required = false;                                              \
        newconfig->magic_num = 6;                                                    \
        newconfig->info = _info;                                                     \
        _pConfigManager.configs[key] = newconfig;                                    \
    } while (0)

#define PARSE_CONFIGS(xml)                 \
    do {                                   \
        _pConfigManager.ParseConfigs(xml); \
    } while (0)

namespace ztool {

struct ConfigValueBase {
    virtual ~ConfigValueBase() = default;
    int magic_num;  // 1 as float, 2 as double
                    // 3 as int, 4 as long, 5 as bool and 6 as string

    unsigned char* m_value;
    bool is_required;
    std::string info;
    void Copy(const ConfigValueBase* rhs) {
        is_required = rhs->is_required;
        magic_num = rhs->magic_num;
        switch (magic_num) {
            case 1:
                memcpy(m_value, rhs->m_value, sizeof(float));
                break;
            case 2:
                memcpy(m_value, rhs->m_value, sizeof(double));
                break;
            case 3:
                memcpy(m_value, rhs->m_value, sizeof(int32_t));
                break;
            case 4:
                memcpy(m_value, rhs->m_value, sizeof(int64_t));
                break;
            case 5:
                memcpy(m_value, rhs->m_value, sizeof(bool));
                break;
            case 6:
                m_value = (unsigned char*)(new std::string(*(std::string*)rhs->m_value));
                break;
            default:
                break;
        }
    }

    std::string Output() {
        char out[32];
        memset(out, 0, 32);
        switch (magic_num) {
            case 1:
                sprintf(out, "%f", *(float*)m_value);
                return out;
            case 2:
                sprintf(out, "%f", *(double*)m_value);
                return out;
            case 3:
                sprintf(out, "%d", *(int*)m_value);
                return out;
            case 4:
                sprintf(out, "%ld", *(long*)m_value);
                return out;
            case 5:
                if (*(bool*)m_value)
                    return "true";
                else
                    return "false";
            case 6:
                return *(std::string*)(m_value);
            default:
                break;
        }
        return out;
    }

    void Clear() { m_value = nullptr; }
};

template <typename T>
struct ConfigValue : ConfigValueBase {
    ConfigValue(const ConfigValue& rhs) {
        m_value = nullptr;
        is_required = rhs.is_required;
        magic_num = rhs.magic_num;
    }

    void Set(T val) { *(T*)m_value = val; }
    T Get() { return *((T*)m_value); }
    ConfigValue(T* val, bool required, T deft) {
        m_value = (unsigned char*)val;
        is_required = required;
        new (m_value) T();
        *(T*)m_value = deft;
    }
};

struct ConfigManager {
    char* data;
    std::string ID;
    std::map<std::string, ConfigValueBase*> configs;
    std::string eg_xml;
    std::string eg_info;
    std::string author;
    std::string email;
    ConfigManager() = default;

    // dtor
    virtual ~ConfigManager() = default;

    const char* _ParseMagicNumber(int num) {
        switch (num) {
            case 1:
                return "FLOAT";
            case 2:
                return "DOUBLE";
            case 3:
                return "INT32";
            case 4:
                return "INT64";
            case 5:
                return "BOOL";
            case 6:
                return "STRING";
            default:
                break;
        }
        return "UNKNOWN";
    }

    void PrintConfigs() {
        printf("SUMMARY: %s\n", eg_info.c_str());
        printf("MAINTAINER: %s \n", author.c_str());
        printf("EMAIL: %s\n", email.c_str());
        printf("Module Example XML: %s\n", eg_xml.c_str());
        printf("IN TOTAL %lu CONFIGS\n", configs.size());
        FOR_EACH_ENUMERATE(k, iter, configs) {
            printf("----------------------------------------\n");
            if (iter->second->is_required) {
                printf("CONFIG %2lu [%8s]: [TYPE:%6s] %s \n", k + 1, "REQUIRED",
                       _ParseMagicNumber(iter->second->magic_num), iter->first.c_str());
                printf("         [RECOMMEND]: %s\n", iter->second->Output().c_str());
                printf("              [INFO]: %s\n", iter->second->info.c_str());
            } else {
                printf("CONFIG %2lu [%8s]: [TYPE:%6s] %s\n", k + 1, "OPTIONAL",
                       _ParseMagicNumber(iter->second->magic_num), iter->first.c_str());
                printf("           [DEFAULT]: %s\n", iter->second->Output().c_str());
                printf("              [INFO]: %s\n", iter->second->info.c_str());
            }
        }
    }

    void ParseConfigs(const ztool::XmlNode* xmlconfig) {
        auto config_map = xmlconfig->getAllAttr();
        auto& config_list = configs;
        FOR_EACH(iter, config_list) {
            auto iter_find = config_map.find(iter->first);
            if (iter_find == config_map.end()) {
                if (iter->second->is_required)
                    LOG_AND_THROW(+"%s required attribute %s is missing", ID.c_str(), iter->first.c_str());
            } else {
                if (iter->second->magic_num == 1) {  // float
                    auto cv = dynamic_cast<ConfigValue<float>*>(iter->second);
                    cv->Set(std::stof(config_map[iter->first]));
                } else if (iter->second->magic_num == 2) {  // double
                    auto cv = dynamic_cast<ConfigValue<double>*>(iter->second);
                    cv->Set(std::stof(config_map[iter->first]));
                } else if (iter->second->magic_num == 3) {  // int
                    auto cv = dynamic_cast<ConfigValue<int>*>(iter->second);
                    cv->Set(std::stoi(config_map[iter->first]));
                } else if (iter->second->magic_num == 4) {  // int
                    auto cv = dynamic_cast<ConfigValue<long>*>(iter->second);
                    cv->Set(std::stoi(config_map[iter->first]));
                } else if (iter->second->magic_num == 5) {  // bool
                    auto cv = dynamic_cast<ConfigValue<bool>*>(iter->second);
                    cv->Set(config_map[iter->first] == "true");
                } else if (iter->second->magic_num == 6) {  // string
                    auto cv = dynamic_cast<ConfigValue<std::string>*>(iter->second);
                    cv->Set(config_map[iter->first]);
                }
                config_map.erase(iter_find);
            }
        }
    }
};

}  // namespace ztool

#endif
