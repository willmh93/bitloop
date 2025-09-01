cmake_minimum_required(VERSION 3.21)
project(bitloop VERSION 0.9 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# ────────── Bootstrap package helpers for both in‑tree & installed modes ──────────
include(bitloopSimulation)


# ──────────────────────────────────────────────────────────────────
# ──────────────────────  Framework Sources  ───────────────────────
# ──────────────────────────────────────────────────────────────────

set(BL_INC "${CMAKE_CURRENT_SOURCE_DIR}/include/bitloop")
set(BL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/bitloop.h"
    "${BL_SRC}/bitloop.cpp"
    "${BL_INC}/core/*.h"
    "${BL_SRC}/core/*.cpp"
)

file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS
    "${BL_INC}/platform/*.h"
    "${BL_SRC}/platform/*.cpp"
)

file(GLOB UTILITY_SOURCES CONFIGURE_DEPENDS
    "${BL_SRC}/util/*.cpp"
    "${BL_INC}/util/*.h"
)

file(GLOB IMGUIX_SOURCES CONFIGURE_DEPENDS
    "${BL_SRC}/imguix/*.cpp"
    "${BL_INC}/imguix/*.h"
)

file(GLOB NANOVGX_SOURCES CONFIGURE_DEPENDS
    "${BL_SRC}/nanovgx/*.cpp"
    "${BL_INC}/nanovgx/*.h"
)

file(GLOB EXTERNAL_HEADERS CONFIGURE_DEPENDS
    "${BL_INC}/3rd_party/*.hpp"
    "${BL_INC}/3rd_party/*.h"
    "${BL_INC}/3rd_party/nanosvg/*.h"
    "${BL_INC}/3rd_party/glad/*.h"
    "${BL_INC}/3rd_party/glad/KHR/*.h"
    "${BL_INC}/3rd_party/nanovg/*.h"
    "${BL_INC}/3rd_party/concurrentqueue/*.h"
)

file(GLOB EXTERNAL_SOURCES CONFIGURE_DEPENDS
    "${BL_SRC}/3rd_party/*.cpp"
    "${BL_SRC}/3rd_party/*.c"
    "${BL_SRC}/3rd_party/glad/*.c"
    "${BL_SRC}/3rd_party/nanovg/*.c"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#source_group("CMakeLists (main)"    FILES ${CMAKE_SOURCE_DIR}/CMakeLists.txt)
#source_group("3rd party"            FILES ${EXTERNAL_HEADERS} ${EXTERNAL_SOURCES})
source_group("Core"                  FILES ${CORE_SOURCES})
source_group("Platform"              FILES ${PLATFORM_SOURCES})
source_group("Utility"               FILES ${UTILITY_SOURCES})
source_group("ImGui (extras)"        FILES ${IMGUIX_SOURCES})
source_group("NanoVG (extras)"       FILES ${NANOVGX_SOURCES})
source_group("Simulations"           FILES ${SIMULATION_HEADERS} ${SIMULATION_SOURCES})

# ──────── Mark all .c files as c++ ────────
foreach(_cfile IN LISTS EXTERNAL_SOURCES)
  if(_cfile MATCHES "\\.c$")
    set_source_files_properties(${_cfile} PROPERTIES LANGUAGE CXX)
  endif()
endforeach()


set(ALL_SRC_FILES
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UTILITY_SOURCES}
    ${IMGUIX_SOURCES}
    ${NANOVGX_SOURCES}

    ${EXTERNAL_HEADERS}
    ${EXTERNAL_SOURCES}
)

add_library(bitloop STATIC ${ALL_SRC_FILES})
_apply_common_settings(bitloop)

#target_precompile_headers(bitloop PUBLIC <bitloop.h>)

target_include_directories(bitloop
  PUBLIC
    # Consumers (and your tests) see only the root, so they must use <bitloop/...>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/bitloop/3rd_party>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/bitloop/3rd_party/glad>
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/bitloop>
    ${CMAKE_CURRENT_SOURCE_DIR}/src

    $<INSTALL_INTERFACE:include/bitloop/3rd_party/glad>
)

# ──────────────────────────────────────────────────────────────────
# ─────────────────────────  Dependencies  ─────────────────────────
# ──────────────────────────────────────────────────────────────────

find_package(Freetype CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
#find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(unofficial-concurrentqueue CONFIG REQUIRED)
find_package(nanovg CONFIG REQUIRED)
find_package(NanoSVG CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-brotli CONFIG REQUIRED)
find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")

target_link_libraries(bitloop
  PUBLIC
    Freetype::Freetype
    imgui::imgui
    SDL3::SDL3
    #glad::glad
    glm::glm
    unofficial::concurrentqueue::concurrentqueue
    nanovg::nanovg
    NanoSVG::nanosvg
    NanoSVG::nanosvgrast
    nlohmann_json::nlohmann_json
    unofficial::brotli::brotlidec
    unofficial::brotli::brotlienc
)
target_include_directories(bitloop PRIVATE
    ${BSHOSHANY_THREAD_POOL_INCLUDE_DIRS}
)

if (NOT EMSCRIPTEN)
    find_package(FFMPEG REQUIRED COMPONENTS avcodec avformat avutil swscale)
    target_link_libraries(bitloop
        PUBLIC
        ${FFMPEG_LIBRARIES})
endif()


# # Link platform GL like most projects do (vcpkg leaves this to the consumer)
# if (WIN32 AND NOT EMSCRIPTEN)
#   target_link_libraries(bitloop PUBLIC opengl32)
# elseif (NOT EMSCRIPTEN)
#   find_package(OpenGL REQUIRED)
#   target_link_libraries(bitloop PUBLIC OpenGL::GL)
# endif()


add_library(bitloop::bitloop ALIAS bitloop)


# ──────── Install static libs ────────
install(TARGETS bitloop
    EXPORT bitloopTargets
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin
)

# ──────── Install public headers ────────
install(DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include/" # trailing slash important to avoid nesting another "include/"
    DESTINATION include
    FILES_MATCHING
      PATTERN "*.h"
      PATTERN "*.hpp"
)

# ──────── Export CMake config file for find_package() ────────
install(EXPORT bitloopTargets
    NAMESPACE bitloop::
    DESTINATION share/bitloop
)

# ──────── Install framework main entry‐point template ────────
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/bitloop_main.cpp"
    DESTINATION share/bitloop/src
    RENAME bitloop_main.cpp
)

# ──────── Install common ────────
if (EMSCRIPTEN)
    install(DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/common"
        DESTINATION share/bitloop
        PATTERN "README.md" EXCLUDE
    )
else()
    # Non-emscripten
    install(DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/common"
        DESTINATION share/bitloop
        PATTERN "README.md" EXCLUDE
        PATTERN "static/" EXCLUDE
        PATTERN "static/*" EXCLUDE
    )
endif()
# ──────── Install simulation cmake helpers ────────
install(FILES
   "${CMAKE_CURRENT_SOURCE_DIR}/bitloopSimulation.cmake"
   DESTINATION share/bitloop
 )

# ──────── Config & Version file ────────
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/bitloopConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/bitloopConfig.cmake"
    INSTALL_DESTINATION share/bitloop)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/bitloopConfigVersion.cmake"
    VERSION  ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/bitloopConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/bitloopConfigVersion.cmake"
    DESTINATION share/bitloop)
