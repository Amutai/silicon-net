add_library(sai SHARED
    src/sai_switch.cpp
    src/sai_port.cpp
    src/sai_fdb.cpp
    src/sai_route.cpp
    src/sai_nexthop.cpp
    src/sai_acl.cpp
)

target_include_directories(sai PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(sai PUBLIC sdk)

if(BUILD_TESTS)
    add_executable(sai_tests
        tests/test_sai.cpp
    )
    target_link_libraries(sai_tests PRIVATE sai GTest::gtest_main)
    gtest_discover_tests(sai_tests)
endif()
