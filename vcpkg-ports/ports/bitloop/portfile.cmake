# portfile.cmake

# 1. Find out where *this* file lives
get_filename_component(_portfile "${CMAKE_CURRENT_LIST_FILE}" ABSOLUTE)
get_filename_component(_portdir  "${_portfile}" DIRECTORY)

# 2. Walk up *exactly* three levels to your bitloop repo root
get_filename_component(BITLOOP_SRC "${_portdir}/../../.." REALPATH)

message(STATUS "––– BITLOOP_SRC = ${BITLOOP_SRC} –––")

# 3. Configure your Bitloop
vcpkg_configure_cmake(
  SOURCE_PATH    "${BITLOOP_SRC}/framework"
  PREFER_NINJA
  OPTIONS
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
)

# 4. Install it into vcpkg’s staging area
vcpkg_install_cmake()

# 5. Export the targets so find_package(bitloop) works
vcpkg_fixup_cmake_targets(CONFIG_PATH share/bitloop)
