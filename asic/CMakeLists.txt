add_library(asic
    src/tables.cpp
    src/port.cpp
    src/pipeline.cpp
)

set_target_properties(asic PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(asic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BUILD_TESTS)
    add_executable(asic_tests
        tests/test_tables.cpp
        tests/test_port.cpp
        tests/test_pipeline.cpp
    )
    target_link_libraries(asic_tests PRIVATE asic GTest::gtest_main)
    gtest_discover_tests(asic_tests)
endif()
