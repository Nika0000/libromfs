#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace test {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& get_tests() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int& test_count() {
    static int count = 0;
    return count;
}

inline int& failed_count() {
    static int count = 0;
    return count;
}

inline void register_test(const std::string& name, std::function<void()> func) {
    get_tests().push_back({name, func});
}

inline int run_all_tests() {
    std::cout << "\n=== Running " << get_tests().size() << " tests ===" << std::endl;

    for (const auto& test : get_tests()) {
        std::cout << "Running test: " << test.name << "..." << std::endl;
        test_count()++;
        try {
            test.func();
            std::cout << "  ✓ PASSED" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "  ✗ FAILED: " << e.what() << std::endl;
            failed_count()++;
        }
    }

    std::cout << "\nResults: " << (test_count() - failed_count()) << "/" << test_count() << " tests passed" << std::endl;

    if (failed_count() > 0) {
        std::cout << "FAILURE: " << failed_count() << " test(s) failed!" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS: All tests passed!" << std::endl;
    return 0;
}

#define TEST(name) \
    void test_##name(); \
    struct test_registrar_##name { \
        test_registrar_##name() { \
            test::register_test(#name, test_##name); \
        } \
    } test_registrar_instance_##name; \
    void test_##name()

#define ASSERT(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error(message); \
    }

#define ASSERT_EQ(a, b, message) \
    if ((a) != (b)) { \
        throw std::runtime_error(std::string(message) + " (expected: " + std::to_string(b) + ", got: " + std::to_string(a) + ")"); \
    }

#define ASSERT_STR_EQ(a, b, message) \
    if (std::string(a) != std::string(b)) { \
        throw std::runtime_error(std::string(message) + " (expected: '" + std::string(b) + "', got: '" + std::string(a) + "')"); \
    }

} // namespace test
