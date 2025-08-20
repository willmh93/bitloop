cmake_minimum_required(VERSION 3.21)
project(examples LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

bitloop_add_dependency(Mandelbrot)
bitloop_add_dependency(Tiger)
bitloop_add_dependency(ExampleText)
#bitloop_add_dependency(Test1)
#bitloop_add_dependency(Test2)

# Add examples target executable
bitloop_new_project(examples)
