include(FetchContent)
FetchContent_Declare(
        googletest
        DOWNLOAD_EXTRACT_TIMESTAMP
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(
        unit_tests
        owned_ptr_test.cpp
        # Foo.cpp
        # Bar.cpp
)

target_link_libraries(unit_tests
        PRIVATE
        gtest_main
)

target_include_directories(unit_tests
        PRIVATE
        ../src
)

add_test(NAME tests COMMAND unit_tests)