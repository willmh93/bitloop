set(VCPKG_ENV_PASSTHROUGH_UNTRACKED EMSCRIPTEN_ROOT EMSDK PATH)
set(CMAKE_SIZEOF_VOID_P 4 CACHE INTERNAL "Override for WebAssembly build" FORCE)

message(STATUS "@@@@@@@@@@@@ Using custom wasm32-emscripten triplet from: ${CMAKE_CURRENT_LIST_FILE}")

if(NOT DEFINED ENV{EMSCRIPTEN_ROOT})
   find_path(EMSCRIPTEN_ROOT "emcc")
else()
   set(EMSCRIPTEN_ROOT "$ENV{EMSCRIPTEN_ROOT}")
endif()

if(NOT EMSCRIPTEN_ROOT)
   if(NOT DEFINED ENV{EMSDK})
      message(FATAL_ERROR "The emcc compiler not found in PATH")
   endif()
   set(EMSCRIPTEN_ROOT "$ENV{EMSDK}/upstream/emscripten")
endif()

if(NOT EXISTS "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
   message(FATAL_ERROR "Emscripten.cmake toolchain file not found")
endif()

set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")

# 3) export to emcc (optional if you push via VCPKG_CMAKE_CONFIGURE_OPTIONS)
#set(ENV{EMCC_JSOPT_BLACKLIST} "*" CACHE STRING "" FORCE)
set(ENV{EMCC_CFLAGS}   "$ENV{EMCC_CFLAGS}   -pthread")
set(ENV{EMCC_CXXFLAGS} "$ENV{EMCC_CXXFLAGS} -pthread")
