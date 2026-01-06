cmake_minimum_required(VERSION 3.16)

project(NetworkLibraryTests LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(NetworkCoreTests
    Test_MessageFramer.cpp
    Test_RingBuffer.cpp
    Test_SendBuffer.cpp
)

target_link_libraries(NetworkCoreTests
    PRIVATE
        NetworkCore
        GTest::gtest_main
)

 target_compile_features(NetworkCoreTests PRIVATE cxx_std_20)

 include(GoogleTest)
 gtest_discover_tests(NetworkCoreTests)