cmake_minimum_required(VERSION 3.16)

project(bitloop LANGUAGES C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0169 OLD)

if (MSVC)
    add_compile_options(/MP) # Multi-threaded compilation
    add_compile_options(/permissive- /W3 /WX /D_CRT_SECURE_NO_WARNINGS)
    #add_compile_options(/Bt+) # optinal: Show compile-time reporting
endif()

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

file(GLOB UI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.h"
)

file(GLOB GRAPHICS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/*.h"
)

file(GLOB EXTERNAL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/KHR/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/nanovg/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/nanovg/*.h"
)

source_group("3rd party"            FILES ${EXTERNAL_SOURCES})
source_group("CMakeLists (main)"    FILES ${CMAKE_SOURCE_DIR}/CMakeLists.txt)
source_group("Core"                 FILES ${CORE_SOURCES})
source_group("Graphics"             FILES ${GRAPHICS_SOURCES})
source_group("ImGui (custom)"       FILES ${UI_SOURCES})
source_group("Simulations"          FILES ${SIMULATION_HEADERS} ${SIMULATION_SOURCES})


add_executable(bitloop
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${GRAPHICS_SOURCES}
    ${SIMULATION_HEADERS}
    ${SIMULATION_SOURCES}
    ${EXTERNAL_SOURCES}
)

target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/simulations)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/nanovg)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad)
target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/KHR)


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

# Include directories
include_directories(${CMAKE_SYSROOT}/include ${CMAKE_SYSROOT}/include/SDL2)
target_include_directories(bitloop PRIVATE
    ${CMAKE_SYSROOT}/include
    ${CMAKE_SYSROOT}/include/SDL2
)

if(WIN32)
    target_link_libraries(bitloop PRIVATE opengl32)
endif()

# ~~~~~ Data Management ~~~~~

if (NOT EMSCRIPTEN)
    file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
endif()

# ~~~~~ Webassembly ~~~~~

if (EMSCRIPTEN)
    message(STATUS "Linking emscripten SDL2")

    # Link SDL2
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    # Compilation flags
    target_compile_options(bitloop PRIVATE
        "-sUSE_PTHREADS=1"
        "-pthread"          # Recommended: expands to -matomics and -mbulk-memory
        "-matomics"
        "-mbulk-memory"
    )

    target_link_options(bitloop PRIVATE
        "-sUSE_SDL=2"
        "-sUSE_WEBGL2=1"
        "-sFULL_ES3=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sUSE_PTHREADS=1"
        "-sPTHREAD_POOL_SIZE=16"
        "--shell-file=${CMAKE_SOURCE_DIR}/static/bitloop.html"
        "--preload-file=${CMAKE_SOURCE_DIR}/data@/data"
    )

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



function(add_deps)
    set(DEPS_BASE_DIR "${FETCHCONTENT_BASE_DIR}/${BUILD_SYSTEM_NAME}/deps")

    set(deps_SOURCE_DIR "${CMAKE_SOURCE_DIR}/cmake/deps")
    #set(deps_BUILD_DIR  "${DEPS_BASE_DIR}/build")

    # Create a generator-safe suffix (e.g. "Ninja" → "build-Ninja")
    string(REPLACE " " "_" SAFE_GENERATOR "${CMAKE_GENERATOR}")
    set(deps_BUILD_DIR  "${DEPS_BASE_DIR}/build-${SAFE_GENERATOR}")
    set(deps_INSTALL_DIR "${DEPS_BASE_DIR}/install")


    # Make it visible **now** and for the caller
    list(PREPEND CMAKE_PREFIX_PATH "${deps_INSTALL_DIR}")
    list(PREPEND CMAKE_PREFIX_PATH "${deps_INSTALL_DIR}" PARENT_SCOPE)

    # Check if install already exists
    #set(bitloopDeps_CONFIG_PATH "${deps_INSTALL_DIR}/lib/cmake/bitloopDeps/bitloopDepsConfig.cmake")


    file(MAKE_DIRECTORY "${deps_BUILD_DIR}")

    message(STATUS "Executing Deps...")
    if (EMSCRIPTEN)
        set(TOOLCHAIN_FLAGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_SYSTEM_NAME=Emscripten
        )
    else()
        set(TOOLCHAIN_FLAGS "")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND}
                -G "${CMAKE_GENERATOR}"
                -S "${deps_SOURCE_DIR}"
                -B "${deps_BUILD_DIR}"
                -DCMAKE_INSTALL_PREFIX=${deps_INSTALL_DIR}
                ${TOOLCHAIN_FLAGS}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE _cfg_result
    )

    if(NOT _cfg_result EQUAL 0)
        message(FATAL_ERROR "Failed to configure bitloopDeps project")
    endif()

    # Build + install
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${deps_BUILD_DIR}" --target install
        RESULT_VARIABLE _bld_result
    )
    if(NOT _bld_result EQUAL 0)
        message(FATAL_ERROR "Failed to build and install bitloopDeps")
    endif()


    message(STATUS "Findings Deps package...")
    unset(bitloopDeps_DIR CACHE)

    find_package(bitloopDeps CONFIG REQUIRED
                 PATHS "${deps_INSTALL_DIR}"
                 NO_DEFAULT_PATH           # only look at the path we give
                 NO_CMAKE_FIND_ROOT_PATH   # do NOT prepend the sys‑root
    )

    if (EMSCRIPTEN)
        target_link_libraries(bitloop PRIVATE
            deps::freetype
            deps::imgui_freetype
            deps::glm
        )
    else()
        target_link_libraries(bitloop PRIVATE
            deps::SDL2-static
            deps::SDL2main
            deps::freetype
            deps::imgui_freetype
            deps::glm
        )
    endif()

    # Add custom target + filter
    include("${deps_BUILD_DIR}/imgui_sources.cmake")

    set(DEPS_CMAKE_FILES
        "${CMAKE_SOURCE_DIR}/cmake/deps/CMakeLists.txt"
        "${CMAKE_SOURCE_DIR}/cmake/deps/DepsConfig.cmake.in"
    )

    add_custom_target(bitloop_deps SOURCES
        ${DEPS_CMAKE_FILES}
        ${IMGUI_SOURCES}
    )

    source_group("CMakeLists (deps)" FILES ${DEPS_CMAKE_FILES})
    source_group("ImGui" FILES ${IMGUI_SOURCES})
endfunction()

message(STATUS "Fetching and building external dependencies...")
add_deps()

include(GNUInstallDirs)
install(TARGETS bitloop
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
