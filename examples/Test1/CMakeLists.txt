cmake_minimum_required(VERSION 3.21)
project(Test1 LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/tooling/cmake/findBitloop.cmake")

# ---------------------------------  flags  ----------------------------------

# if this Cmake project open as root, BITLOOP_DEV_MODE overrides child projects
# set(BITLOOP_DEV_MODE TRUE)

# ------------------  configure project hierarchy/sources  -------------------


file(GLOB SIM_SOURCES CONFIGURE_DEPENDS
	"Test1/*.cpp"
	"Test1/*.h"
)

# 2. Create new project
bitloop_new_project(Test1  ${SIM_SOURCES})

bitloop_finalize()
