cmake_minimum_required(VERSION 3.21)
project({SIM_NAME} LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS "{SIM_NAME}/*.cpp" "{SIM_NAME}/*.h")

# Create new project
bitloop_new_project({SIM_NAME} ${SIM_SOURCES})

# Add dependency projects
#bitloop_add_dependency({SIM_NAME}  "./path/to/project")

bitloop_finalize()
