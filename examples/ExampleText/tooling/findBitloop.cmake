# tooling/findBitloop.cmake
include_guard(GLOBAL)

macro(bitloop_enable_languages)
  if (BITLOOP_DISCOVERY)
    # discovery must continue so bitloop_finalize can emit discovered_manifests.cmake
  else()
    set(_args ${ARGN})

    # allow: bitloop_enable_languages(LANGUAGES C CXX)
    if (_args)
      list(GET _args 0 _a0)
      if (_a0 STREQUAL "LANGUAGES")
        list(REMOVE_AT _args 0)
      else()
        # allow: bitloop_enable_languages(<name> LANGUAGES C CXX)
        list(LENGTH _args _n)
        if (_n GREATER 1)
          list(GET _args 1 _a1)
          if (_a1 STREQUAL "LANGUAGES")
            list(REMOVE_AT _args 0 1)
          endif()
        endif()
      endif()
    endif()

    foreach(_lang IN LISTS _args)
      if (NOT CMAKE_${_lang}_COMPILER_LOADED)
        enable_language(${_lang})
      endif()
    endforeach()

    if (CMAKE_CXX_COMPILER_LOADED)
      include(CheckCXXCompilerFlag)
    endif()
  endif()
endmacro()

get_property(bl_in_try_compile GLOBAL PROPERTY IN_TRY_COMPILE)
if(bl_in_try_compile)
  return()
endif()

# tooling -> project root
get_filename_component(bl_project_root "${CMAKE_CURRENT_LIST_DIR}/.." REALPATH)

if(BITLOOP_DISCOVERY)
  include("${CMAKE_CURRENT_LIST_DIR}/toolchain/discovery_shims.cmake")
  return()
endif()

if(NOT TARGET bitloop::bitloop)
  if(EXISTS "${bl_project_root}/bitloop/CMakeLists.txt")
    add_subdirectory("${bl_project_root}/bitloop" "${CMAKE_BINARY_DIR}/_bitloop")
  else()
    find_package(bitloop CONFIG REQUIRED)
  endif()
endif()
