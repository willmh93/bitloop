cmake_minimum_required(VERSION 3.21)
project(ExampleText LANGUAGES CXX)

# Link to bitloop
if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

# Gather source files for this project
file(GLOB SIM_SOURCES CONFIGURE_DEPENDS
	"ExampleText/*.cpp"
	"ExampleText/*.h"
)

# Create new project and set dependencies
# (in the order you want them to appear in the UI)
bitloop_new_project(ExampleText ${SIM_SOURCES})

bitloop_finalize()
