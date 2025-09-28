cmake_minimum_required(VERSION 3.21)
project(MandelbrotGallery LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS "MandelbrotGallery/*.cpp" "MandelbrotGallery/*.h")

# Create new project
bitloop_new_project(MandelbrotGallery ${SIM_SOURCES})

# Add dependency projects
bitloop_add_dependency(MandelbrotGallery  "../Mandelbrot")

bitloop_finalize()
