cmake_minimum_required(VERSION 3.16)

project(bitloop LANGUAGES C CXX)
include(FetchContent)

set(CMAKE_PROJECT_NAME BL)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0169 OLD)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(apply_common_settings target)

    target_compile_features(${target} PUBLIC cxx_std_23)
    target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:DEBUG>)

    if (MSVC)
        target_compile_options(${target} PRIVATE /MP) # Multi-threaded compilation
        target_compile_options(${target} PRIVATE /permissive- /W3 /WX /D_CRT_SECURE_NO_WARNINGS)
        #add_compile_options(/Bt+) # optinal: Show compile-time reporting
    endif()
    
    if (EMSCRIPTEN)
        # Compilation flags
        target_compile_options(${target} PRIVATE
            "-sUSE_PTHREADS=1"
            "-pthread"
            "-matomics"
            "-mbulk-memory"
        )

        target_link_options(${target} PRIVATE
            "-sUSE_SDL=3"
            "-sUSE_WEBGL2=1"
            "-sFULL_ES3=1"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sUSE_PTHREADS=1"
            "-sPTHREAD_POOL_SIZE=16"
            "--shell-file=${CMAKE_SOURCE_DIR}/static/bitloop.html"
            "--preload-file=${CMAKE_SOURCE_DIR}/data@/data"
        )
    endif()
endfunction()

function(apply_fast_settings target)
    apply_common_settings(${target})
    
    #if(IS_MSVC)
    #    target_compile_options(${target} PRIVATE /fp:fast)
    #elseif (IS_EMSCRIPTEN)
    #    target_compile_options(${target} PRIVATE
    #        -msimd128                      # if you want SIMD
    #        -fno-math-errno                # disables setting errno
    #        -freciprocal-math              # allow reciprocal simplifications
    #        -fassociative-math             # allow reassociating expressions
    #        -fno-signed-zeros              # allow x + 0.0 ≠ x
    #        -fno-trapping-math             # allow traps to be optimized out
    #        -fno-rounding-math             # no rounding-mode sensitivity
    #        -fno-unsafe-math-optimizations
    #        -mllvm -enable-emscripten-sjlj # enables better support for some pragmas
    #    )
    #else()
    #    target_compile_options(${target} PRIVATE -ffast-math)
    #endif()

endfunction()



# sets default startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT bitloop)

#file(GLOB SIMULATION_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/simulations/*.h")
#file(GLOB SIMULATION_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/simulations/*.cpp")
#list(SORT SIMULATION_HEADERS)
#list(SORT SIMULATION_SOURCES)

# ~~~~~ File globs ~~~~~

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
    #"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    #"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    #"${CMAKE_CURRENT_SOURCE_DIR}/src/main_window.cpp"
    #"${CMAKE_CURRENT_SOURCE_DIR}/src/main_window.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.h"
)

file(GLOB UTILITY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utility/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utility/*.h"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/nanosvg/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/KHR/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/nanovg/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/nanovg/*.h"
)

# Grab simulations directories (each owns its own CMakeLists.txt)
file(GLOB CHILD_SIM_DIRS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/simulations/*)

source_group("3rd party"            FILES ${EXTERNAL_SOURCES})
source_group("CMakeLists (main)"    FILES ${CMAKE_SOURCE_DIR}/CMakeLists.txt)
source_group("Core"                 FILES ${CORE_SOURCES})
source_group("Utility"              FILES ${UTILITY_SOURCES})
source_group("Graphics"             FILES ${GRAPHICS_SOURCES})
source_group("ImGui (custom)"       FILES ${UI_SOURCES})
source_group("Simulations"          FILES ${SIMULATION_HEADERS} ${SIMULATION_SOURCES})

#add_executable(bitloop
#    ${CORE_SOURCES}
#    ${UTILITY_SOURCES}
#    ${UI_SOURCES}
#    ${GRAPHICS_SOURCES}
#    ${SIMULATION_HEADERS}
#    ${SIMULATION_SOURCES}
#    ${EXTERNAL_SOURCES}
#)

#if (EMSCRIPTEN)
#    target_compile_options(bitloop PRIVATE -msimd128)
#    target_link_options(bitloop PRIVATE -msimd128)
#elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
#    if(MSVC)
#        target_compile_options(bitloop PRIVATE /arch:AVX2)
#    else()
#        target_compile_options(bitloop PRIVATE -mavx2)
#    endif()
#endif()

#if(MSVC)
#    target_compile_options(bitloop PRIVATE /fp:fast)
#    #target_compile_options(bitloop PRIVATE /fp:contract)
#    
#else()
#    target_compile_options(bitloop PRIVATE -ffast-math)
#    #target_compile_options(bitloop PRIVATE -ffp-contract=fast)
#endif()

#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/utility)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/simulations)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/nanovg)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad)
#target_include_directories(bitloop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/KHR)




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


# ─── Helper: link common third-party deps to a target ──────────────────────────
function(link_common_deps target)
    if (EMSCRIPTEN)
        target_link_libraries(${target} PRIVATE
            deps::freetype 
            deps::imgui_freetype 
            deps::glm)
    else()
        target_link_libraries(${target} PRIVATE
            deps::SDL3-static
            deps::freetype
            deps::imgui_freetype
            deps::glm)
    endif()

    # Preprocessor for WebGL
    target_compile_definitions(${target} PRIVATE 
        IMGUI_IMPL_OPENGL_ES2
        NANOVG_IMPLEMENTATION)
endfunction()

function(register_simulation target)
    # Register the target globally
    set_property(GLOBAL APPEND PROPERTY SIM_MODULES ${target})

    # Apply VS solution folder
    set_target_properties(${target} PROPERTIES FOLDER "Simulations")

    # Make its headers accessible to consumers
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # Link core + third-party deps
    target_link_libraries(${target} PRIVATE SimulationCore)

    link_common_deps(${target})

    get_target_property(_srcs ${target} SOURCES)
    if(_srcs)
        source_group("src" FILES ${_srcs})
    endif()
endfunction()

#function(register_simulation target)
#    # Register the object library target globally (for later collection)
#    set_property(GLOBAL APPEND PROPERTY SIM_MODULES ${target})
#
#    # Apply VS solution folder
#    set_target_properties(${target} PROPERTIES FOLDER "Simulations")
#
#    # Mark the simulation's headers as publicly accessible
#    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#
#    # Link against SimulationCore (needed even for object libs to resolve symbols)
#    target_link_libraries(${target} PRIVATE SimulationCore)
#
#    # Link any third-party dependencies (e.g. SDL, freetype, etc.)
#    link_common_deps(${target})
#
#    # Validate that this is an OBJECT library
#    get_target_property(lib_type ${target} TYPE)
#    if(NOT lib_type STREQUAL "OBJECT_LIBRARY")
#        message(WARNING "Simulation target '${target}' is not an OBJECT library. Consider using add_library(${target} OBJECT ...) instead of STATIC.")
#    endif()
#endfunction()


function(build_deps)
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
build_deps()
#link_deps();

# ───────────────────────────────────────────────────────────────────────────────
# 2. Core static library – compiled only once
# ───────────────────────────────────────────────────────────────────────────────
add_library(SimulationCore STATIC
    ${CORE_SOURCES}
    ${UTILITY_SOURCES}
    ${UI_SOURCES}
    ${GRAPHICS_SOURCES}
    ${EXTERNAL_SOURCES}
)

target_include_directories(SimulationCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utility
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulations
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/nanovg
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad
    ${CMAKE_CURRENT_SOURCE_DIR}/src/3rd_party/glad/KHR
)


link_common_deps(SimulationCore)


# ───────────────────────────────────────────────────────────────────────────────
# 3. Add every simulation sub-project (each sets its own options)
# ───────────────────────────────────────────────────────────────────────────────

#set(ALL_SIM_TARGETS "")
#foreach (SIM_DIR ${CHILD_SIM_DIRS})
#    if (IS_DIRECTORY ${SIM_DIR} AND EXISTS "${SIM_DIR}/CMakeLists.txt")
#        # Clear the cache variable before entering the subdir
#        unset(SIM_TARGET CACHE)
#        unset(SIM_TARGET)
#
#        add_subdirectory(${SIM_DIR})
#
#        if(SIM_TARGET)
#            list(APPEND ALL_SIM_TARGETS ${SIM_TARGET})
#        endif()
#    endif()
#endforeach()

set(ALL_SIM_TARGETS "")
foreach (SIM_DIR ${CHILD_SIM_DIRS})
    if (IS_DIRECTORY ${SIM_DIR} AND EXISTS "${SIM_DIR}/CMakeLists.txt")
        add_subdirectory(${SIM_DIR})
        list(APPEND ALL_SIM_TARGETS ${SIM_TARGET})
    endif()
endforeach()



# pull the list back from the global property
get_property(ALL_SIM_TARGETS GLOBAL PROPERTY SIM_MODULES)



############################

# Generate simulations_registry.h with ForceLink functions
set(SIM_REGISTRY_HEADER "${CMAKE_BINARY_DIR}/generated/simulations_registry.h")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
file(WRITE "${SIM_REGISTRY_HEADER}" "#pragma once\n\n")
file(APPEND "${SIM_REGISTRY_HEADER}" "extern \"C\" {\n")
foreach(SIM_NAME ${ALL_SIM_TARGETS})
    file(APPEND "${SIM_REGISTRY_HEADER}" "    void ${SIM_NAME}_ForceLink();\n")
endforeach()
file(APPEND "${SIM_REGISTRY_HEADER}" "}\n\n")

file(APPEND "${SIM_REGISTRY_HEADER}" "inline void force_link_all_simulations() {\n")
foreach(SIM_NAME ${ALL_SIM_TARGETS})
    file(APPEND "${SIM_REGISTRY_HEADER}" "    ${SIM_NAME}_ForceLink();\n")
endforeach()
file(APPEND "${SIM_REGISTRY_HEADER}" "}\n")


# ───────────────────────────────────────────────────────────────────────────────
# 4. Final executable  ----------------------------------------------------------
#     – only main.cpp (or a tiny wrapper) lives here; all heavy code is in libs
# ───────────────────────────────────────────────────────────────────────────────

add_executable(bitloop src/main.cpp)        # adjust if your entry-point lives elsewhere

target_link_libraries(bitloop PUBLIC  
    SimulationCore 
    #${ALL_SIM_TARGETS}
)

foreach(SIM_NAME ${ALL_SIM_TARGETS})
    target_sources(bitloop PRIVATE $<TARGET_OBJECTS:${SIM_NAME}>)
endforeach()

#target_link_libraries(bitloop PUBLIC SimulationCore)

target_include_directories(bitloop PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(bitloop PRIVATE "${CMAKE_BINARY_DIR}/generated")

apply_common_settings(bitloop)
apply_common_settings(SimulationCore)

link_common_deps(bitloop)


if (MSVC)
    set_target_properties(bitloop PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(WIN32)
    target_link_libraries(bitloop PRIVATE opengl32)
endif()

# ~~~~~ Data Management ~~~~~

if (NOT EMSCRIPTEN)
    file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
endif()

# ~~~~~ Webassembly ~~~~~

if (EMSCRIPTEN)
    message(STATUS "Linking emscripten SDL3")

    # Link SDL3
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    ## Compilation flags
    #target_compile_options(bitloop PRIVATE
    #    "-sUSE_PTHREADS=1"
    #    "-pthread"          # Recommended: expands to -matomics and -mbulk-memory
    #    "-matomics"
    #    "-mbulk-memory"
    #)
    #
    #target_link_options(bitloop PRIVATE
    #    "-sUSE_SDL=3"
    #    "-sUSE_WEBGL2=1"
    #    "-sFULL_ES3=1"
    #    "-sALLOW_MEMORY_GROWTH=1"
    #    "-sUSE_PTHREADS=1"
    #    "-sPTHREAD_POOL_SIZE=16"
    #    "--shell-file=${CMAKE_SOURCE_DIR}/static/bitloop.html"
    #    "--preload-file=${CMAKE_SOURCE_DIR}/data@/data"
    #)

    include_directories($ENV{EMSDK}/upstream/emscripten/cache/sysroot/include)
endif()

include(GNUInstallDirs)
install(TARGETS bitloop
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
