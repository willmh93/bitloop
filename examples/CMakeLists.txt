cmake_minimum_required(VERSION 3.21)
project(examples LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/tooling/findBitloop.cmake")

# Add examples target executable
bitloop_new_project(examples)
bitloop_add_dependency(examples "./Tiger")
bitloop_add_dependency(examples "./ExampleText")
bitloop_finalize()
