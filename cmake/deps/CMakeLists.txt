cmake_minimum_required(VERSION 3.16)
project(bitloopDeps LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(FETCHCONTENT_TRY_ENABLE_BY_DEFAULT OFF)
set(CMAKE_INSTALL_MESSAGE NEVER)


# ─────────────────────────  SDL3  ─────────────────────────
if (NOT EMSCRIPTEN)
    message(STATUS "Fetching SDL3...")

    # Disable build tests
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)

    # Important: Use the d-suffix for Debug builds
    set(SDL_CMAKE_DEBUG_POSTFIX "" CACHE STRING "" FORCE)

    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.2.16
    )
    FetchContent_MakeAvailable(SDL3)

    # Install SDL3 library targets
    install(TARGETS SDL3-static SDL3_Headers  EXPORT depsTargets)

    # Install SDL3 headers manually
    install(DIRECTORY ${SDL3_SOURCE_DIR}/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    add_library(deps::SDL3-static ALIAS SDL3-static)
else()
    add_compile_options(
        "-sUSE_SDL=3"
        "-sUSE_PTHREADS=1"
        "-pthread"          # Recommended: expands to -matomics and -mbulk-memory
        "-matomics"
        "-mbulk-memory"
    )

    add_compile_options(-pthread -matomics -mbulk-memory)
    add_link_options(-pthread -sUSE_PTHREADS=1 -sALLOW_MEMORY_GROWTH=1)

    # ---- Include paths --------------------------------------------------------
    # 1) Real SDL headers that ship in the sysroot
    target_include_directories(${target} PRIVATE
        "$ENV{EMSDK}/upstream/emscripten/cache/sysroot/include"
    )

    # 2) Add one-line alias header so <SDL3/SDL.h> resolves
    set(SDL3_ALIAS_DIR "${CMAKE_BINARY_DIR}/sdl3_alias/include/SDL3")
    file(MAKE_DIRECTORY "${SDL3_ALIAS_DIR}")
    file(WRITE "${SDL3_ALIAS_DIR}/SDL.h"
        "#pragma once\n#include <SDL/SDL.h>\n")

    target_include_directories(${target} PRIVATE
        "${CMAKE_BINARY_DIR}/sdl3_alias/include"
    )
endif()



# ────────────────────────  FreeType  ──────────────────────
FetchContent_Declare(freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
    GIT_TAG        VER-2-13-1)

# disable every optional dependency *before* populating
set(FT_WITH_ZLIB      OFF CACHE BOOL "" FORCE)
set(FT_WITH_BZip2     OFF CACHE BOOL "" FORCE)
set(FT_WITH_PNG       OFF CACHE BOOL "" FORCE)
set(FT_WITH_BROTLI    OFF CACHE BOOL "" FORCE)
set(FT_WITH_HarfBuzz  OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(freetype)
install(TARGETS freetype EXPORT depsTargets)



# ───────────── ImGui (+FreeType) ─────────────
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking)
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui) # get imgui_SOURCE_DIR

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
)

add_library(imgui_freetype STATIC ${IMGUI_SOURCES})

file(WRITE "${CMAKE_BINARY_DIR}/imgui_sources.cmake" "set(IMGUI_SOURCES\n")
foreach(src IN LISTS IMGUI_SOURCES)
    file(APPEND "${CMAKE_BINARY_DIR}/imgui_sources.cmake" "    \"${src}\"\n")
endforeach()
file(APPEND "${CMAKE_BINARY_DIR}/imgui_sources.cmake" ")\n")


# link SDL3/freetype to ImGui
if(NOT EMSCRIPTEN)
    target_link_libraries(imgui_freetype PUBLIC freetype SDL3-static)
else()
    target_link_libraries(imgui_freetype PUBLIC freetype)
endif()

# SDL headers for the Emscripten tool‑chain
if (EMSCRIPTEN)
    # inside bitloopDeps CMakeLists.txt  (only for EMSCRIPTEN)
    #get_filename_component(EMSDK_SYSROOT "${CMAKE_SYSROOT}" ABSOLUTE)

    #find_path(EMSCRIPTEN_SDL2_INCLUDE
    #          NAMES SDL.h
    #          PATHS "${EMSDK_SYSROOT}/include/SDL"
    #          REQUIRED)
    #
    #message(STATUS "EMSCRIPTEN_SDL2_INCLUDE: ${EMSCRIPTEN_SDL2_INCLUDE}")
    include_directories($ENV{EMSDK}/upstream/emscripten/cache/sysroot/include)
endif()

target_include_directories(imgui_freetype PUBLIC
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/imgui>)

install(TARGETS imgui_freetype EXPORT depsTargets)
install(DIRECTORY ${imgui_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui
        FILES_MATCHING PATTERN "*.h")

# ─────────────────────────   GLM  ─────────────────────────
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)
FetchContent_GetProperties(glm)

# Real installable target (no colons)
add_library(deps_glm INTERFACE)
target_include_directories(deps_glm INTERFACE
    $<BUILD_INTERFACE:${glm_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Exported alias target (namespaced)
add_library(deps::glm ALIAS deps_glm)

# Install the real one — the alias will work from find_package
install(TARGETS deps_glm EXPORT depsTargets)

# Install headers so INSTALL_INTERFACE works
install(DIRECTORY "${glm_SOURCE_DIR}/glm"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")



# ────────────────────────  Export set  ─────────────────────
install(EXPORT depsTargets
        FILE bitloopDepsTargets.cmake
        NAMESPACE deps::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitloopDeps)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/DepsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/bitloopDepsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitloopDeps)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/bitloopDepsConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitloopDeps)



