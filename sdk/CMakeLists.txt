add_library(sdk
    src/sdk.cpp
)

target_include_directories(sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sdk PUBLIC asic)

if(BUILD_TESTS)
    add_executable(sdk_tests
        tests/test_sdk.cpp
    )
    target_link_libraries(sdk_tests PRIVATE sdk GTest::gtest_main)
    gtest_discover_tests(sdk_tests)
endif()
