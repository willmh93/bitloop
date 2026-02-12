cmake_minimum_required(VERSION 3.21)
project(examples LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

# Add examples target executable
bitloop_new_project(examples)

bitloop_add_dependency(examples "./Tiger")
bitloop_add_dependency(examples "./ExampleText")

bitloop_finalize()
