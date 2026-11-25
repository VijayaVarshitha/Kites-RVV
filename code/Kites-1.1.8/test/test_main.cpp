#include <gtest/gtest.h>
#include <cstdlib>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include <QLoggingCategory>

int main(int argc, char *argv[])
{
#ifdef _WIN32
    // Set console output code page to UTF-8 (65001)
    SetConsoleOutputCP(65001);
#endif
    ::testing::InitGoogleTest(&argc, argv);

    // The simulator prints a line for every step it executes ("Program Counter: ...",
    // "VM_STEP_COMPLETED", "Cycle: ..."). Over ~1,700 tests that is millions of lines and
    // makes the suite crawl. GoogleTest reports through printf, not std::cout, so silencing
    // std::cout keeps every [ RUN ] / [ OK ] / [ FAILED ] line and the final summary.
    // Set KITES_TEST_VERBOSE=1 to see the simulator's step output again.
    const char *verbose = std::getenv("KITES_TEST_VERBOSE");
    if (verbose == nullptr || verbose[0] == '\0' || verbose[0] == '0')
        std::cout.setstate(std::ios_base::failbit);

    return RUN_ALL_TESTS();
}
