#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <sstream>

namespace test {

struct TestCase {
    std::string name;
    std::string suite;
    std::function<void()> func;
};

class TestRunner {
public:
    static TestRunner& instance() {
        static TestRunner r;
        return r;
    }

    void register_test(const std::string& suite, const std::string& name, std::function<void()> func) {
        tests_.push_back({name, suite, func});
    }

    int run_all() {
        int passed = 0;
        int failed = 0;
        std::string current_suite = "";

        std::cout << "\033[1;36m====================================================\033[0m\n";
        std::cout << "\033[1;36m       WARLOCK SIMULATOR UNIT TEST SUITE            \033[0m\n";
        std::cout << "\033[1;36m====================================================\033[0m\n\n";

        for (const auto& test : tests_) {
            if (test.suite != current_suite) {
                current_suite = test.suite;
                std::cout << "\033[1;33m[" << current_suite << "]\033[0m\n";
            }

            current_test_failed_ = false;
            current_failures_.clear();

            try {
                test.func();
            } catch (const std::exception& ex) {
                current_test_failed_ = true;
                current_failures_.push_back(std::string("Unhandled exception: ") + ex.what());
            } catch (...) {
                current_test_failed_ = true;
                current_failures_.push_back("Unhandled unknown exception");
            }

            if (current_test_failed_) {
                failed++;
                std::cout << "  \033[1;31m[FAIL]\033[0m " << test.name << "\n";
                for (const auto& err : current_failures_) {
                    std::cout << "         \033[31m-> " << err << "\033[0m\n";
                }
            } else {
                passed++;
                std::cout << "  \033[1;32m[PASS]\033[0m " << test.name << "\n";
            }
        }

        std::cout << "\n\033[1;36m----------------------------------------------------\033[0m\n";
        std::cout << "Total: " << (passed + failed) 
                  << " | \033[1;32mPassed: " << passed << "\033[0m"
                  << " | \033[1;" << (failed > 0 ? "31m" : "32m") << "Failed: " << failed << "\033[0m\n";
        std::cout << "\033[1;36m====================================================\033[0m\n";

        return failed > 0 ? 1 : 0;
    }

    void record_failure(const std::string& msg) {
        current_test_failed_ = true;
        current_failures_.push_back(msg);
    }

private:
    std::vector<TestCase> tests_;
    bool current_test_failed_ = false;
    std::vector<std::string> current_failures_;
};

struct AutoTestRegister {
    AutoTestRegister(const std::string& suite, const std::string& name, std::function<void()> func) {
        TestRunner::instance().register_test(suite, name, func);
    }
};

} // namespace test

#define TEST_CASE(suite, name) \
    static void test_##suite##_##name(); \
    static test::AutoTestRegister reg_##suite##_##name(#suite, #name, test_##suite##_##name); \
    static void test_##suite##_##name()

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::ostringstream ss; \
            ss << "Check failed: " << #expr << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            test::TestRunner::instance().record_failure(ss.str()); \
        } \
    } while (0)

#define CHECK_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            std::ostringstream ss; \
            ss << "Equality failed: " << #actual << " == " << #expected \
               << " (actual: " << (actual) << ", expected: " << (expected) \
               << ") at " << __FILE__ << ":" << __LINE__; \
            test::TestRunner::instance().record_failure(ss.str()); \
        } \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance) \
    do { \
        if (std::fabs((actual) - (expected)) > (tolerance)) { \
            std::ostringstream ss; \
            ss << "Tolerance check failed: |" << #actual << " - " << #expected << "| <= " << #tolerance \
               << " (actual: " << (actual) << ", expected: " << (expected) \
               << ", diff: " << std::fabs((actual) - (expected)) << ") at " << __FILE__ << ":" << __LINE__; \
            test::TestRunner::instance().record_failure(ss.str()); \
        } \
    } while (0)
