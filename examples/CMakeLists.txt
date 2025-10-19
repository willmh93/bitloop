cmake_minimum_required(VERSION 3.21)
project(examples LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

# Add examples target executable
bitloop_new_project(examples)

bitloop_add_dependency(examples "./Tiger")
bitloop_add_dependency(examples "./ExampleText")
#bitloop_add_dependency(examples "./Test1")

#bitloop_hide(Cardioid)
#bitloop_show(Cardioid)

#bitloop_show_only(Mandelbrot)
#bitloop_hide(Tiger)
#bitloop_hide(ExampleText)



bitloop_finalize()

#
#bitloop_add_dependency(Test2)
