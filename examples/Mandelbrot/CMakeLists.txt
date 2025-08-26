cmake_minimum_required(VERSION 3.21)
project(Mandelbrot  LANGUAGES CXX)

# Link to bitloop
if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()


file(GLOB SIM_SOURCES CONFIGURE_DEPENDS "Mandelbrot/*.cpp" "Mandelbrot/*.h")

bitloop_add_dependency(../Cardioid)

# 2. Create new project
bitloop_new_project(Mandelbrot  ${SIM_SOURCES})
