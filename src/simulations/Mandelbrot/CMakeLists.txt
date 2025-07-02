set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ====== Define Target Name ============================

get_filename_component(SIM_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

# ====== Simulation Source Files =======================

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
add_library(${SIM_NAME} OBJECT ${SIM_SOURCES})


# ====== Compiler Flags ================================

apply_fast_settings(${SIM_NAME})

if (EMSCRIPTEN)
    target_compile_options(${SIM_NAME} PRIVATE -msimd128)
    target_link_options(${SIM_NAME} PRIVATE -msimd128)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    if(MSVC)
        target_compile_options(${SIM_NAME} PRIVATE /arch:AVX2)
    else()
        target_compile_options(${SIM_NAME} PRIVATE -mavx2)
    endif()
endif()


# ====== Register Sim ==================================

register_simulation(${SIM_NAME})