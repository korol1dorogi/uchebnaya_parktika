# Standalone-проект C++ юнит-тестов: НЕ зависит от Drogon.
# Тестирует чистые утилиты из ../utils через GoogleTest.
#   cmake -S tests -B build-tests && cmake --build build-tests && ctest --test-dir build-tests
cmake_minimum_required(VERSION 3.16)
project(praktika_tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(praktika_tests test_utils.cc)
target_include_directories(praktika_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../utils)
target_link_libraries(praktika_tests PRIVATE GTest::gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(praktika_tests)
