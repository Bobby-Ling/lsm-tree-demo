add_executable(lsm_tests
    lsm_tests.cpp
)

target_link_libraries(lsm_tests external gtest_main)
