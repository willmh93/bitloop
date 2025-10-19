cmake_minimum_required(VERSION 3.21)
project(Test2 LANGUAGES CXX)

# Link to bitloop
if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS "Test2/*.cpp" "Test2/*.h")

# 1. Setup project dependencies (if any)
bitloop_add_dependency(../Test1)

# 2. Create new project
bitloop_new_project(Test2 ${SIM_SOURCES})
