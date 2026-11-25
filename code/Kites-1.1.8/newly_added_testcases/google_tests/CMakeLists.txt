# Google Tests for the vector extension. They run the same programs as the manual tests
# (../manual_tests/*.s) on the single-cycle processor and two pipelines and check the results.
include(GoogleTest)
find_package(Threads REQUIRED)

add_executable(kites_vector_tests rvv_vector_tests.cpp)
target_link_libraries(kites_vector_tests PRIVATE kites_core GTest::gtest GTest::gtest_main Threads::Threads)
if(DISABLE_GUI)
    target_compile_definitions(kites_vector_tests PRIVATE DISABLE_GUI)
endif()

gtest_discover_tests(kites_vector_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DISCOVERY_MODE PRE_TEST
    PROPERTIES FAIL_REGULAR_EXPRESSION ".*FAILED.*")
