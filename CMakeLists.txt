cmake_minimum_required(VERSION 3.16)

project(bitloop LANGUAGES C CXX)
include(FetchContent)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MESSAGE_LOG_LEVEL WARNING)

# sets default startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT bitloop)

file(GLOB SIMULATION_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/simulations/*.h")
file(GLOB SIMULATION_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/simulations/*.cpp")
list(SORT SIMULATION_HEADERS)
list(SORT SIMULATION_SOURCES)

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.h"
)

file(GLOB GRAPHICS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/glad/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/glad/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/glad/KHR/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nanovg/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nanovg/*.h"
)

add_executable(bitloop
    ${CORE_SOURCES}
    ${SIMULATION_HEADERS}
    ${SIMULATION_SOURCES}
    ${GRAPHICS_SOURCES}
   # src/main.cpp
)

target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/nanovg)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/glad)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/glad/KHR)


if (MSVC)
    set_target_properties(bitloop PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Preprocessor for WebGL
target_compile_definitions(bitloop PRIVATE
    IMGUI_IMPL_OPENGL_ES2
    NANOVG_IMPLEMENTATION
)

if (EMSCRIPTEN)
    target_compile_definitions(bitloop PRIVATE NANOVG_GLES3_IMPLEMENTATION)
else()
    target_compile_definitions(bitloop PRIVATE NANOVG_GL3_IMPLEMENTATION)
endif()

# Include directories
include_directories(${CMAKE_SYSROOT}/include ${CMAKE_SYSROOT}/include/SDL2)
target_include_directories(bitloop PRIVATE
    ${CMAKE_SYSROOT}/include
    ${CMAKE_SYSROOT}/include/SDL2
)

if(WIN32)
    target_link_libraries(bitloop PRIVATE opengl32)
endif()

# ~~~~~ Webassembly ~~~~~

if (EMSCRIPTEN)
    # Link SDL2
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    add_compile_options("-sUSE_SDL=2")
    #target_compile_options(bitloop PRIVATE
    #    "-sUSE_SDL=2"
    #)
    target_link_options(bitloop PRIVATE
        "-sUSE_SDL=2"
        "-sUSE_WEBGL2=1"
        "-sFULL_ES3=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sUSE_PTHREADS=1"
        "-sPTHREAD_POOL_SIZE=16"
        "--shell-file=${CMAKE_SOURCE_DIR}/static/bitloop_local.html"
    )


    #target_compile_options(bitloop PRIVATE "-Wno-unused-command-line-argument")
    #set(EMSDK_COMMON_FLAGS
    #    "-sUSE_SDL=2"           # SDL2 with mixer, image, etc. if present
    #    "-sUSE_WEBGL2=1"        # request WebGL2 context
    #    "-sFULL_ES3=1"          # expose all GLES3 functions
    #    "-sALLOW_MEMORY_GROWTH" # grow heap when needed
    #    "-sPTHREAD_POOL_SIZE=4" # example threading flag
    #)

    # ---------------- Compile flags ----------------
    #   - forward the Emscripten switches
    #   - disable the “ignored during compilation” warning they trigger
    #target_compile_options(bitloop PRIVATE
    #    ${EMSDK_COMMON_FLAGS}
    #    "-Wno-unused-command-line-argument"
    #)

    # ---------------- Link flags -------------------
    #   - same ‑s… flags again
    #   - anything that is *link‑only* (e.g. shell file)
    #target_link_options(bitloop PRIVATE
    #    ${EMSDK_COMMON_FLAGS}
    #    "--shell-file=${CMAKE_SOURCE_DIR}/static/bitloop_local.html"
    #)

    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/static/bitloop_local.html -s USE_SDL=2 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=16")


    #add_link_options(
    #    "-sUSE_WEBGL2=1"
    #    "-sFULL_ES3=1"
    #    "-sALLOW_MEMORY_GROWTH=1"
    #    "-sPTHREAD_POOL_SIZE=4"
    #    "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html"
    #)

    include_directories($ENV{EMSDK}/upstream/emscripten/cache/sysroot/include)
endif()

# ~~~~~ External Fetched ~~~~~

# Detect build system
if (CMAKE_GENERATOR MATCHES "Ninja")
    set(BUILD_SYSTEM_NAME "ninja")
elseif (CMAKE_GENERATOR MATCHES "Visual Studio")
    set(BUILD_SYSTEM_NAME "vs")
else()
    set(BUILD_SYSTEM_NAME "other") # Fallback
endif()
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/build_fetched)

#message(STATUS "CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
#message(STATUS "FETCHCONTENT_BASE_DIR: ${FETCHCONTENT_BASE_DIR}")
#message(STATUS "BUILD_SYSTEM_NAME: ${BUILD_SYSTEM_NAME}")
#message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
#message(STATUS "CONFIG: ${CONFIG}")

function(add_sdl2)
    set(SDL2_BASE_DIR ${FETCHCONTENT_BASE_DIR}/${BUILD_SYSTEM_NAME}/SDL2)

    if (NOT EMSCRIPTEN)
        set(SDL2_SOURCE_DIR  "${SDL2_BASE_DIR}/src")
        set(SDL2_BUILD_DIR   "${SDL2_BASE_DIR}/build")
        set(SDL2_INSTALL_DIR "${SDL2_BASE_DIR}/install")
        set(SDL2_SUBBUILD_DIR "${SDL2_BASE_DIR}/subbuild")

        # Declare SDL2 but do not add to build tree
        FetchContent_Declare(SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.30.0
            SOURCE_DIR ${SDL2_SOURCE_DIR}
            BINARY_DIR ${SDL2_BUILD_DIR}
            SUBBUILD_DIR ${SDL2_SUBBUILD_DIR}
        )

        # Download SDL2 if not already
        FetchContent_GetProperties(SDL2)
        if(NOT SDL2_POPULATED)
            message(STATUS "Fetching SDL2...")
            FetchContent_Populate(SDL2)
        endif()

        # Guard to prevent rebuild
        if(NOT SDL2_ALREADY_BUILT AND NOT EXISTS "${SDL2_INSTALL_DIR}/cmake/SDL2/SDL2Config.cmake")
            message(STATUS "Building SDL2 for the first time...")

            set(SDL2_ALREADY_BUILT TRUE CACHE BOOL "Prevent rebuilding SDL2")

            execute_process(COMMAND ${CMAKE_COMMAND}
                -S ${SDL2_SOURCE_DIR}
                -B ${SDL2_BUILD_DIR}
                -DCMAKE_INSTALL_PREFIX=${SDL2_INSTALL_DIR}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DSDL_TEST=OFF
                -DSDL_SHARED=ON
                RESULT_VARIABLE configure_result
            )
            if(NOT configure_result EQUAL 0)
                message(FATAL_ERROR "Failed to configure SDL2")
            endif()

            execute_process(COMMAND ${CMAKE_COMMAND}
                --build ${SDL2_BUILD_DIR}
                --target install
                RESULT_VARIABLE build_result
            )
            if(NOT build_result EQUAL 0)
                message(FATAL_ERROR "Failed to build and install SDL2")
            endif()
        endif()

        # Point CMake to installed config
        set(SDL2_DIR "${SDL2_INSTALL_DIR}/cmake")
        find_package(SDL2 REQUIRED)

        target_link_libraries(bitloop PRIVATE SDL2::SDL2 SDL2::SDL2main)

        add_custom_command(TARGET bitloop POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL2::SDL2>
                    $<TARGET_FILE_DIR:bitloop>
        )
    endif()
endfunction()

function(add_imgui)
    if(EMSCRIPTEN)
        # For WASM builds, just add ImGui directly as a static lib in-tree
        message(STATUS "Adding ImGui directly for Emscripten build")

        set(IMGUI_DIR "${FETCHCONTENT_BASE_DIR}/${BUILD_SYSTEM_NAME}/imgui/src")

        add_library(imgui STATIC
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp
            ${IMGUI_DIR}/imgui_demo.cpp
            ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
            ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        )

        target_include_directories(imgui PUBLIC
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
        )

        target_link_libraries(bitloop PRIVATE imgui)

        return()
    endif()

    set(IMGUI_BASE_DIR ${FETCHCONTENT_BASE_DIR}/${BUILD_SYSTEM_NAME}/imgui)
    set(IMGUI_SOURCE_DIR "${IMGUI_BASE_DIR}/src")
    set(IMGUI_BUILD_DIR  "${IMGUI_BASE_DIR}/build")
    set(IMGUI_INSTALL_DIR "${IMGUI_BASE_DIR}/install")
    set(IMGUI_CONFIG_PATH "${IMGUI_INSTALL_DIR}/lib/cmake/imgui/imguiConfig.cmake")
    set(SDL2_DIR "${FETCHCONTENT_BASE_DIR}/${BUILD_SYSTEM_NAME}/SDL2/install/cmake")

    # Only download the source (can be your wrapper or original repo)
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
        SOURCE_DIR ${IMGUI_SOURCE_DIR}
    )
    FetchContent_MakeAvailable(imgui)

    # Install the wrapped version (only once)
    if(NOT EXISTS "${IMGUI_CONFIG_PATH}")
        message(STATUS "Building ImGui wrapper for first time...")
        execute_process(COMMAND ${CMAKE_COMMAND}
            -S "${CMAKE_SOURCE_DIR}/cmake/imgui"
            -B ${IMGUI_BUILD_DIR}
            -DCMAKE_INSTALL_PREFIX=${IMGUI_INSTALL_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DSDL2_DIR=${SDL2_DIR}  # So it can find SDL2::SDL2
            -DIMGUI_SRC_DIR=${IMGUI_SOURCE_DIR}
        )
        execute_process(COMMAND ${CMAKE_COMMAND}
            --build ${IMGUI_BUILD_DIR} --target install
        )
    endif()

    # Make ImGui available
    set(imgui_DIR "${IMGUI_INSTALL_DIR}/lib/cmake/imgui")
    find_package(imgui CONFIG REQUIRED)

    target_link_libraries(bitloop PRIVATE imgui::imgui)
endfunction()

#function(add_imgui)
#    set(IMGUI_BASE_DIR ${FETCHCONTENT_BASE_DIR}/${BUILD_SYSTEM_NAME}/imgui)
#    set(IMGUI_SOURCE_DIR "${IMGUI_BASE_DIR}/src")
#    set(IMGUI_POPULATE_DIR "${IMGUI_BASE_DIR}/subbuild")
#
#    # Only fetch once
#    if(NOT IMGUI_ALREADY_POPULATED)
#        set(IMGUI_ALREADY_POPULATED TRUE CACHE BOOL "Prevent re-fetching ImGui")
#        FetchContent_Declare(
#            imgui
#            GIT_REPOSITORY https://github.com/ocornut/imgui.git
#            GIT_TAG docking #v1.91.9
#            SOURCE_DIR ${IMGUI_SOURCE_DIR}
#            BINARY_DIR "${IMGUI_BASE_DIR}/build"
#            SUBBUILD_DIR ${IMGUI_POPULATE_DIR}
#        )
#        FetchContent_Populate(imgui)
#    endif()
#
#    # Avoid redefining the target every time
#    if(NOT TARGET imgui)
#        add_library(imgui STATIC
#            ${IMGUI_SOURCE_DIR}/imconfig.h
#            ${IMGUI_SOURCE_DIR}/imgui.h
#            ${IMGUI_SOURCE_DIR}/imgui.cpp
#            ${IMGUI_SOURCE_DIR}/imgui_internal.h
#            ${IMGUI_SOURCE_DIR}/imstb_rectpack.h
#            ${IMGUI_SOURCE_DIR}/imstb_textedit.h
#            ${IMGUI_SOURCE_DIR}/imstb_truetype.h
#            ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
#            ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
#            ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
#            ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
#            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.h
#            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
#            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl2.h
#            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
#        )
#
#        # Optional for IDE support
#        target_sources(imgui PRIVATE
#            ${IMGUI_SOURCE_DIR}/imgui.h
#            ${IMGUI_SOURCE_DIR}/imconfig.h
#            ${IMGUI_SOURCE_DIR}/imgui_internal.h
#            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.h
#            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl2.h
#        )
#
#        # Includes and linkage
#        target_include_directories(imgui PUBLIC
#            ${IMGUI_SOURCE_DIR}
#            ${IMGUI_SOURCE_DIR}/backends
#        )
#
#        if(NOT EMSCRIPTEN)
#            target_link_libraries(imgui PUBLIC SDL2::SDL2 SDL2::SDL2main)
#        endif()
#    endif()
#
#    target_link_libraries(bitloop PRIVATE imgui)
#endfunction()

#function(add_imgui)
#    set(IMGUI_BUILD_DIR ${FETCHCONTENT_BASE_DIR}/${BUILD_SYSTEM_NAME}/imgui)
#    FetchContent_Declare(
#        imgui
#        GIT_REPOSITORY https://github.com/ocornut/imgui.git
#        GIT_TAG docking #v1.91.9
#    )
#    FetchContent_Populate(
#        imgui
#        SOURCE_DIR ${IMGUI_BUILD_DIR}/src
#        BINARY_DIR ${IMGUI_BUILD_DIR}/build
#        SUBBUILD_DIR ${IMGUI_BUILD_DIR}/subbuild
#    )
#
#    add_library(imgui STATIC
#        ${imgui_SOURCE_DIR}/imconfig.h
#        ${imgui_SOURCE_DIR}/imgui.h
#        ${imgui_SOURCE_DIR}/imgui.cpp
#        ${imgui_SOURCE_DIR}/imgui_internal.h
#        ${imgui_SOURCE_DIR}/imstb_rectpack.h
#        ${imgui_SOURCE_DIR}/imstb_textedit.h
#        ${imgui_SOURCE_DIR}/imstb_truetype.h
#        ${imgui_SOURCE_DIR}/imgui_draw.cpp
#        ${imgui_SOURCE_DIR}/imgui_tables.cpp
#        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
#        ${imgui_SOURCE_DIR}/imgui_demo.cpp
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.h
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
#    )
#
#    # Add headers only for clarity or IDE support (optional)
#    target_sources(imgui PRIVATE
#        ${imgui_SOURCE_DIR}/imgui.h
#        ${imgui_SOURCE_DIR}/imconfig.h
#        ${imgui_SOURCE_DIR}/imgui_internal.h
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.h
#    )
#
#    # Make ImGui headers available to targets that link to it
#    target_include_directories(imgui PUBLIC
#        ${imgui_SOURCE_DIR}
#        ${imgui_SOURCE_DIR}/backends
#    )
#
#    if (NOT EMSCRIPTEN)
#        target_link_libraries(imgui PUBLIC SDL2::SDL2 SDL2::SDL2main)
#    endif()
#
#    target_link_libraries(bitloop PRIVATE imgui)
#endfunction()

add_sdl2()
add_imgui()

include(GNUInstallDirs)
install(TARGETS bitloop
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


