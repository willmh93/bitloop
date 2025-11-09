cmake_minimum_required(VERSION 3.21)
project({SIM_NAME} LANGUAGES CXX)

if (NOT TARGET bitloop::bitloop)
  find_package(bitloop CONFIG REQUIRED)
endif()

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS "{SIM_NAME}/*.cpp" "{SIM_NAME}/*.h")

# Create new project
bitloop_new_project({SIM_NAME} ${SIM_SOURCES})

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

# Add dependency projects
#bitloop_add_dependency({SIM_NAME}  "./path/to/project")

bitloop_finalize()
