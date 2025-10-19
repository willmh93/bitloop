cmake_minimum_required(VERSION 3.21)
project(Test1  LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS
	"Test1/*.cpp"
	"Test1/*.h"
)

# 2. Create new project
bitloop_new_project(Test1  ${SIM_SOURCES})

bitloop_finalize()
