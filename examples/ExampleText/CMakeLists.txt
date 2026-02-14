cmake_minimum_required(VERSION 3.21)
project(ExampleText LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/tooling/findBitloop.cmake")

# ---------------------------------  flags  ----------------------------------

# if this Cmake project open as root, BITLOOP_DEV_MODE overrides child projects
# set(BITLOOP_DEV_MODE TRUE)

# ------------------  configure project hierarchy/sources  -------------------

# Gather source files for this project
file(GLOB SIM_SOURCES CONFIGURE_DEPENDS
	"ExampleText/*.cpp"
	"ExampleText/*.h"
)

# Create new project and set dependencies
# (in the order you want them to appear in the UI)
bitloop_new_project(ExampleText ${SIM_SOURCES})

bitloop_finalize()
