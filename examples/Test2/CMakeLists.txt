cmake_minimum_required(VERSION 3.21)
project(Test2 LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/tooling/cmake/findBitloop.cmake")


# ---------------------------------  flags  ----------------------------------

# if this Cmake project open as root, BITLOOP_DEV_MODE overrides child projects
# set(BITLOOP_DEV_MODE TRUE)

# ------------------  configure project hierarchy/sources  -------------------


file(GLOB SIM_SOURCES CONFIGURE_DEPENDS "Test2/*.cpp" "Test2/*.h")

# 1. Setup project dependencies (if any)
bitloop_add_dependency(../Test1)

# 2. Create new project
bitloop_new_project(Test2 ${SIM_SOURCES})
