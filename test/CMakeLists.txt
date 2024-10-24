project(tests)

set(test1_source main.cpp)
add_executable(test1 ${test1_source})
target_link_libraries(test1 sgc GTest::gtest_main )

include(GoogleTest)
gtest_discover_tests(test1)