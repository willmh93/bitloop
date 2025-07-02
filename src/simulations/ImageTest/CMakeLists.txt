# ====== Define Target Name ============================

get_filename_component(SIM_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

# ====== Simulation Source Files =======================

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
add_library(${SIM_NAME} OBJECT ${SIM_SOURCES})


# ====== Compiler Flags ================================

apply_fast_settings(${SIM_NAME})


# ====== Register Sim ==================================

register_simulation(${SIM_NAME})