cmake_minimum_required(VERSION 3.21)
project(Tiger  LANGUAGES CXX)

# Link to bitloop
if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS
	"Tiger/*.cpp"
	"Tiger/*.h")

bitloop_new_project(Tiger  ${SIM_SOURCES})

bitloop_finalize()
