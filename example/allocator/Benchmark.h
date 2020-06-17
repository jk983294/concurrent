#ifndef CONCURRENT_BENCHMARK_H
#define CONCURRENT_BENCHMARK_H

#include <chrono>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <random>
#include <set>
#include <unordered_map>
#include "allocator/FastPoolAllocator.h"

using namespace std;

constexpr int numberOfIterations = 1024;

class PerformanceTimer {
    std::chrono::high_resolution_clock::time_point from, to;

public:
    void start() { from = std::chrono::high_resolution_clock::now(); }

    void stop() { to = std::chrono::high_resolution_clock::now(); }

    double toSeconds() const { return std::chrono::duration_cast<std::chrono::duration<double>>(to - from).count(); }
};

template <typename Alloc>
class Performance {
    template <typename Container>
    double test_push_front() {
        Container container;
        PerformanceTimer timer;
        timer.start();

        for (int i = 0; i < numberOfIterations; i++) {
            int size = 0;
            while (size < numberOfIterations) container.push_front(size++);
            for (; size > numberOfIterations; size--) container.pop_front();
        }

        timer.stop();
        return timer.toSeconds();
    }

    template <typename Container>
    double test_push_back() {
        Container container;
        PerformanceTimer timer;
        timer.start();

        for (int i = 0; i < numberOfIterations; i++) {
            int size = 0;
            while (size < numberOfIterations) container.push_back(size++);
            for (; size > numberOfIterations; size--) container.pop_back();
        }

        timer.stop();
        return timer.toSeconds();
    }

    template <typename Container>
    double test_set() {
        Container container;
        PerformanceTimer timer;
        timer.start();

        for (int i = 0; i < numberOfIterations; i++) {
            int size = 0;
            while (size < numberOfIterations) container.insert(size++);
            for (; size > numberOfIterations; size--) container.erase(size);
        }

        timer.stop();
        return timer.toSeconds();
    }

    void runTests() {
        std::cout << "List PushFront - Default STL Allocator : " << std::fixed << test_push_front<std::list<int>>()
                  << " seconds." << std::endl;
        std::cout << "List PushFront - Tested Allocator : " << std::fixed << test_push_front<std::list<int, Alloc>>()
                  << " seconds." << std::endl
                  << std::endl;

        std::cout << "List PushBack - Default STL Allocator : " << std::fixed << test_push_back<std::list<int>>()
                  << " seconds." << std::endl;
        std::cout << "List PushBack - Tested Allocator : " << std::fixed << test_push_back<std::list<int, Alloc>>()
                  << " seconds." << std::endl
                  << std::endl;

        std::cout << "Set - Default STL Allocator : " << std::fixed << test_set<std::set<int, std::less<int>>>()
                  << " seconds." << std::endl;
        std::cout << "Set - Tested Allocator : " << std::fixed << test_set<std::set<int, std::less<int>, Alloc>>()
                  << " seconds." << std::endl
                  << std::endl;
    }

public:
    void run() {
        std::cout << "Allocator performance measurement example" << std::endl;
        runTests();
    }
};


template <typename Alloc>
class MapPerformance {
    template <typename Container>
    double test_map() {
        Container container;
        PerformanceTimer timer;
        timer.start();

        for (int i = 0; i < numberOfIterations; i++) {
            int size = 0;
            while (size < numberOfIterations) container.insert(std::pair<char, int>(size++, size));
            for (; size > numberOfIterations; size--) container.erase(size);
        }

        timer.stop();
        return timer.toSeconds();
    }

    void runTests() {
        std::cout << "Map - Default STL Allocator : " << std::fixed << test_map<std::map<int, int, std::less<int>>>()
                  << " seconds." << std::endl;
        std::cout << "Map - Tested Allocator : " << std::fixed << test_map<std::map<int, int, std::less<int>, Alloc>>()
                  << " seconds." << std::endl
                  << std::endl;

        std::cout << "unordered_map - Default STL Allocator : " << std::fixed
                  << test_map<std::unordered_map<int, int>>() << " seconds." << std::endl;
        std::cout << "unordered_map - Tested Allocator : " << std::fixed
                  << test_map<std::unordered_map<int, int, std::hash<int>, std::equal_to<int>,
                          frenzy::FastPoolAllocator<std::pair<const int, int>>>>()
                  << " seconds." << std::endl
                  << std::endl;
    }

public:
    void run() {
        std::cout << "Allocator performance measurement example" << std::endl;
        runTests();
    }
};

#endif
