# bitloop/CMakeLists.txt

cmake_minimum_required(VERSION 3.21)
project(bitloop-superbuild LANGUAGES CXX)

add_subdirectory(framework)
add_subdirectory(examples)

option(BITLOOP_BUILD_TESTS "Build tests" OFF)
if (BITLOOP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
