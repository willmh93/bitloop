cmake_minimum_required(VERSION 3.21)
project(Tiger LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/tooling/findBitloop.cmake")

# ---------------------------------  flags  ----------------------------------

# if this Cmake project open as root, BITLOOP_DEV_MODE overrides child projects
# set(BITLOOP_DEV_MODE TRUE)

# ------------------  configure project hierarchy/sources  -------------------

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS
	"Tiger/*.cpp"
	"Tiger/*.h")

bitloop_new_project(Tiger  ${SIM_SOURCES})

bitloop_finalize()
