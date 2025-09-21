
if (DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  set(_vcpkg_dir "$ENV{VCPKG_ROOT}")
  message(STATUS "vcpkg toolchain detected at: ${_vcpkg_dir}")
elseif (DEFINED ENV{BITLOOP_ROOT} AND EXISTS "$ENV{BITLOOP_ROOT}/vcpkg/scripts/buildsystems/vcpkg.cmake")
  set(_vcpkg_dir "$ENV{BITLOOP_ROOT}/vcpkg")
  message(STATUS "vcpkg toolchain detected at: ${_vcpkg_dir}")
else()
  message(FATAL_ERROR
    "vcpkg not found.\n"
    "Tried:\n"
    "  - $ENV{VCPKG_ROOT} (if set)\n"
    "  - $ENV{BITLOOP_ROOT}/vcpkg (if set)\n")
endif()

set(_vcpkg_toolchain "${_vcpkg_dir}/scripts/buildsystems/vcpkg.cmake")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

list(APPEND VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-ports/ports")

message(STATUS "Using vcpkg at: ${_vcpkg_dir}")
include("${_vcpkg_toolchain}")


