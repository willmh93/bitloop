cmake_minimum_required(VERSION 3.16)
project(imgui LANGUAGES CXX)

# Instead of hardcoding, let top-level CMake tell us where ImGui is
set(IMGUI_SRC_DIR "" CACHE PATH "Path to fetched ImGui source")


add_library(imgui STATIC
    ${IMGUI_SRC_DIR}/imgui.cpp
    ${IMGUI_SRC_DIR}/imgui_draw.cpp
    ${IMGUI_SRC_DIR}/imgui_tables.cpp
    ${IMGUI_SRC_DIR}/imgui_widgets.cpp
    ${IMGUI_SRC_DIR}/imgui_demo.cpp
    ${IMGUI_SRC_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_SRC_DIR}/backends/imgui_impl_sdl2.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_SRC_DIR}
    ${IMGUI_SRC_DIR}/backends
)

find_package(SDL2 REQUIRED)

target_link_libraries(imgui PUBLIC SDL2::SDL2 SDL2::SDL2main)

# Make installable
include(GNUInstallDirs)
install(TARGETS imgui EXPORT imguiTargets)

install(DIRECTORY ${IMGUI_SRC_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui
    FILES_MATCHING PATTERN "*.h")

install(EXPORT imguiTargets
    FILE imguiTargets.cmake
    NAMESPACE imgui::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/imguiConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)
