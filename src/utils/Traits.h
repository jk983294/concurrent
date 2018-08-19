#ifndef CONCURRENT_TRAITS_H
#define CONCURRENT_TRAITS_H

#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>

namespace frenzy {

    enum TypeIndex : uint8_t {
        null_type,
        char_type,
        char_array_type,
        string_type,
        short_type,
        int_type,
        float_type,
        double_type,
        unsigned_short_type,
        unsigned_int_type,
        int8_type,
        int16_type,
        int32_type,
        int64_type,
        uint8_type,
        uint16_type,
        uint32_type,
        uint64_type,
        vector_type,
        list_type,
        map_type
    };

    template<typename T>
    struct TypeIndexStruct {
        const static TypeIndex type = TypeIndex::null_type;
    };

    template<>
    struct TypeIndexStruct<char> {
        const static TypeIndex type = TypeIndex::char_type;
    };

    template<>
    struct TypeIndexStruct<std::string> {
        const static TypeIndex type = TypeIndex::string_type;
    };

    template<>
    struct TypeIndexStruct<char*> {
        const static TypeIndex type = TypeIndex::string_type;
    };

    template<>
    struct TypeIndexStruct<float> {
        const static TypeIndex type = TypeIndex::float_type;
    };

    template<>
    struct TypeIndexStruct<double> {
        const static TypeIndex type = TypeIndex::double_type;
    };

    template<>
    struct TypeIndexStruct<int8_t> {
        const static TypeIndex type = TypeIndex::int8_type;
    };

    template<>
    struct TypeIndexStruct<int16_t> {
        const static TypeIndex type = TypeIndex::int16_type;
    };

    template<>
    struct TypeIndexStruct<int32_t> {
        const static TypeIndex type = TypeIndex::int32_type;
    };

    template<>
    struct TypeIndexStruct<int64_t> {
        const static TypeIndex type = TypeIndex::int64_type;
    };

    template<>
    struct TypeIndexStruct<uint8_t> {
        const static TypeIndex type = TypeIndex::uint8_type;
    };

    template<>
    struct TypeIndexStruct<uint16_t> {
        const static TypeIndex type = TypeIndex::uint16_type;
    };

    template<>
    struct TypeIndexStruct<uint32_t> {
        const static TypeIndex type = TypeIndex::uint32_type;
    };

    template<>
    struct TypeIndexStruct<uint64_t> {
        const static TypeIndex type = TypeIndex::uint64_type;
    };

    template<typename ValueType, typename AllocType>
    struct TypeIndexStruct<std::vector<ValueType, AllocType>> {
        const static TypeIndex type = TypeIndex::vector_type;
    };

    template<typename ValueType, typename AllocType>
    struct TypeIndexStruct<std::list<ValueType, AllocType>> {
        const static TypeIndex type = TypeIndex::list_type;
    };

    template<template <typename, typename...> class MapType, typename ValueType, typename... Args>
    struct TypeIndexStruct<MapType<ValueType, Args...>> {
        const static TypeIndex type = TypeIndex::map_type;
    };
}

#endif
