cmake_minimum_required(VERSION 3.21)
project(Mandelbrot  LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

# Gather source files for this project
file(GLOB SIM_SOURCES CONFIGURE_DEPENDS
	"Mandelbrot/*.cpp"
	"Mandelbrot/*.h"
)


bitloop_new_project(Mandelbrot  ${SIM_SOURCES})
bitloop_add_dependency(Mandelbrot  "../Cardioid")

bitloop_hide(Cardioid)

bitloop_finalize()
