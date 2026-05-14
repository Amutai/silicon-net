cmake_minimum_required(VERSION 3.16)
project(silicon-net VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)

# Testing
if(BUILD_TESTS)
    enable_testing()
    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif()

# Sub-projects (bottom-up dependency order)
add_subdirectory(asic)
add_subdirectory(sdk)
add_subdirectory(sai)
