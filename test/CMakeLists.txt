include(FetchContent)
FetchContent_Declare(gtest
        QUIET
        DOWNLOAD_EXTRACT_TIMESTAMP
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)

add_executable(
        unit_tests
        owned_ptr_test.cpp
)

target_link_libraries(unit_tests
        PRIVATE
        gtest_main
)

target_include_directories(unit_tests
        PRIVATE
        ../src
)

# automatic discovery of unit tests
include(GoogleTest)
gtest_discover_tests(unit_tests
        PROPERTIES
        LABELS "unit_tests"
        DISCOVERY_TIMEOUT  # how long to wait (in seconds) before crashing
        240
)