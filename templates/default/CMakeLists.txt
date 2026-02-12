cmake_minimum_required(VERSION 3.21)
project({SIM_NAME} LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

# ------------------------------------------------------------------
# -------------- configure project hierarchy/sources ---------------
# ------------------------------------------------------------------

set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/{SIM_NAME}")
file(GLOB_RECURSE SIM_SOURCES CONFIGURE_DEPENDS
	"${SRC_DIR}/*.cpp"
	"${SRC_DIR}/*.h"
	"${SRC_DIR}/*.hpp"
)

# Create new project
bitloop_new_project({SIM_NAME} ${SIM_SOURCES})

# Add dependency projects
#bitloop_add_dependency({SIM_NAME}  "./path/to/project")

 # finalize (project discovery pass stops here)
bitloop_finalize()

# ------------------------------------------------------------------
# ----- configure your target (compile options, packages, etc) -----
# ------------------------------------------------------------------

# fast math
#if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
#	if (MSVC)
#		target_compile_options({SIM_NAME} PRIVATE /arch:AVX2 /fp:fast)
#	else() # GCC/Clang
#		target_compile_options({SIM_NAME} PRIVATE -mavx2 -mfma -ffp-contract=fast -ffast-math -fno-math-errno)
#	endif()
#else()
#	target_compile_options({SIM_NAME} PRIVATE -O3 -flto -msimd128 -mrelaxed-simd -ffast-math -ffp-contract=fast -fno-math-errno)
#endif()

