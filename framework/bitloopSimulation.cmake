# --- bitloopSimulation.cmake ---

set(BITLOOP_MAIN_SOURCE		"${CMAKE_CURRENT_LIST_DIR}/src/bitloop_main.cpp"	CACHE INTERNAL "")
set(BITLOOP_COMMON			"${CMAKE_CURRENT_LIST_DIR}/common"					CACHE INTERNAL "")
set(BITLOOP_BUILD_DIR		"${CMAKE_CURRENT_BINARY_DIR}"						CACHE INTERNAL "")

# Auto-generated include folder
set(BITLOOP_AUTOGEN_DIR		"${CMAKE_CURRENT_BINARY_DIR}/include_autogen" CACHE INTERNAL "")
file(MAKE_DIRECTORY			"${BITLOOP_AUTOGEN_DIR}")
set(AUTOGEN_SIM_INCLUDES	"${BITLOOP_AUTOGEN_DIR}/bitloop_simulations.h" CACHE INTERNAL "")

set(BITLOOP_PROJECT_NAMES ""		CACHE INTERNAL "Ordered included projected")
set(BITLOOP_DATA_DEPENDENCIES ""	CACHE INTERNAL "Ordered list of dependency data directories")

set_property(DIRECTORY PROPERTY BITLOOP_DEPENDENCY_DIRS "")

# Begin auto-generated header file
file(WRITE "${AUTOGEN_SIM_INCLUDES}" "// Auto‑generated simulation includes\n")
file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#include <bitloop/core/project.h>\n\n")

function(apply_common_settings _TARGET)
	target_compile_features(${_TARGET} PUBLIC cxx_std_23)

	if (MSVC)
		target_compile_options(${_TARGET} PRIVATE /MP) # Multi-threaded compilation
		target_compile_options(${_TARGET} PRIVATE /permissive- /W3 /WX /utf-8 /D_CRT_SECURE_NO_WARNINGS)
	elseif (EMSCRIPTEN)
		# O3 for WASM
		target_compile_options(${_TARGET} PRIVATE 
			"-O3"
			"-sUSE_PTHREADS=1"
			"-pthread"
			"-matomics"
			"-mbulk-memory"
		)

		target_link_options(${_TARGET} PRIVATE
			"-pthread"
			"-sUSE_SDL=3"
			"-sUSE_WEBGL2=1"
			"-sFULL_ES3=1"
			"-sALLOW_MEMORY_GROWTH=1"
			"-sUSE_PTHREADS=1"
			"-sPTHREAD_POOL_SIZE=32"
			"-sPTHREADS_DEBUG=1"
			"-sEXPORTED_RUNTIME_METHODS=[ccall,UTF8ToString,stringToUTF8,lengthBytesUTF8]"
		)
	endif()
endfunction()

function(apply_main_settings _TARGET)
	if (MSVC)
		set_target_properties(${_TARGET} PROPERTIES WIN32_EXECUTABLE TRUE)
	elseif (EMSCRIPTEN)
		target_link_options(${_TARGET} PRIVATE
			"--shell-file=${BITLOOP_COMMON}/static/shell.html"
			#"--embed-file=${CMAKE_CURRENT_BINARY_DIR}/data@/data"
			"--embed-file=${CMAKE_SOURCE_DIR}/build/${BUILD_FLAVOR}/app/data@/data"
		)
		set_target_properties(${_TARGET} PROPERTIES
			OUTPUT_NAME "index"
			SUFFIX ".html"
		)
	endif()
endfunction()

function(apply_root_exe_name target)
  # If the preset provided a flavor (single-config like Ninja), use it.
  if(BUILD_FLAVOR)
    set(_root "${CMAKE_SOURCE_DIR}/build/${BUILD_FLAVOR}/app")
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${_root}"
      LIBRARY_OUTPUT_DIRECTORY "${_root}"
      ARCHIVE_OUTPUT_DIRECTORY "${_root}"
    )
  else()
    # Multi-config or no flavor: split by configuration.
    foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
      string(TOUPPER "${cfg}" CFG)
      set(_root "${CMAKE_SOURCE_DIR}/build/${cfg}/app")
      set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CFG} "${_root}"
        LIBRARY_OUTPUT_DIRECTORY_${CFG} "${_root}"
        ARCHIVE_OUTPUT_DIRECTORY_${CFG} "${_root}"
      )
    endforeach()
  endif()
  # Consistent executable "name" regardless of project.
  set_target_properties(${target} PROPERTIES OUTPUT_NAME "app")
endfunction()

macro(bitloop_new_project sim_name)
	# collect the other args as source files
	set(SIM_SOURCES ${ARGN})

	# If root project not yet determined, set current project as root executable
	if (NOT BL_ROOT_PROJECT)
		set(BL_ROOT_PROJECT ${CMAKE_CURRENT_SOURCE_DIR})
	endif()
	
	if (CMAKE_CURRENT_SOURCE_DIR STREQUAL BL_ROOT_PROJECT)
		# top-level (executable) 
		set(_TARGET ${sim_name})

		if(NOT SIM_SOURCES)
			set(SIM_SOURCES ${BITLOOP_MAIN_SOURCE}) # no user sources (e.g. superbuild), just use the framework main()
			set(SIM_SOURCES_PROVIDED FALSE)
		else()
			list(APPEND SIM_SOURCES ${BITLOOP_MAIN_SOURCE}) # sources provided by user, so append the framework main()
			set(SIM_SOURCES_PROVIDED TRUE)
		endif()

		set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${_TARGET}")
		add_executable(${_TARGET} ${SIM_SOURCES})

		apply_root_exe_name(${_TARGET})

		 target_include_directories(${_TARGET} PRIVATE "$<BUILD_INTERFACE:${BITLOOP_AUTOGEN_DIR}>")

	else()
		# nested (library)
		set(_TARGET ${sim_name})
		set(SIM_SOURCES_PROVIDED TRUE)

		add_library(${_TARGET} ${SIM_SOURCES})
		add_library(${sim_name}::${sim_name} ALIAS ${sim_name}) # Export an alias so consumers can link as sim::sim
	endif()


	target_link_libraries(${_TARGET} PRIVATE bitloop::bitloop)


	# Append #include to the auto-generated header
	if (SIM_SOURCES_PROVIDED)
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#pragma push_macro(\"SIM_NAME\")\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#undef SIM_NAME\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#define SIM_NAME ${sim_name}\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#include <${sim_name}/${sim_name}.h>\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "using ${sim_name}::${sim_name}_Project;\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#pragma pop_macro(\"SIM_NAME\")\n\n")
        
        set(_shim "${CMAKE_CURRENT_BINARY_DIR}/.bl_ns_this_project.h")
        file(WRITE "${_shim}" "#pragma once\n#define SIM_NAME ${sim_name}\n")
        
        if (MSVC)
          target_compile_options(${_TARGET} PRIVATE "/FI${_shim}")
        else()
          target_compile_options(${_TARGET} PRIVATE "-include" "${_shim}")
        endif()

	ENDIF()

	if (CMAKE_CURRENT_SOURCE_DIR STREQUAL BL_ROOT_PROJECT)
		msg(STATUS "")
		msg(STATUS "────────── Project Tree ──────────")
		msg(STATUS "[${sim_name}]")
	endif()

	msg_indent_push()

	get_property(_list DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_DEPENDENCY_DIRS)
	foreach(dep_path IN LISTS _list)
		get_filename_component(dep_name ${dep_path} NAME)
		msg(STATUS "[${dep_name}]")
		_bitloop_add_dependency(${_TARGET} ${dep_path})
	endforeach()

	msg_indent_pop()

	# Add project to list (for finalizing later)
	if (SIM_SOURCES_PROVIDED)
		get_property(_project_names GLOBAL PROPERTY BITLOOP_PROJECT_NAMES)
		if (NOT sim_name IN_LIST _project_names)
			list(APPEND _project_names ${sim_name})
		endif()
		set_property(GLOBAL PROPERTY BITLOOP_PROJECT_NAMES ${_project_names})


		# ---- Include Tree (include wrappers that define project namespace macros) ----

		file(GLOB_RECURSE _pub_headers
		  "${CMAKE_CURRENT_SOURCE_DIR}/${sim_name}/*.h"
		  "${CMAKE_CURRENT_SOURCE_DIR}/${sim_name}/*.hpp"
		)

		# Project include wrapper root
		set(_wrap_root "${CMAKE_CURRENT_BINARY_DIR}/include")
		set(_wrap_ns_root "${_wrap_root}/${sim_name}")
		file(MAKE_DIRECTORY "${_wrap_ns_root}")


		# Generate one wrapper per public header
		foreach(_hdr IN LISTS _pub_headers)
		  # path relative to project root
		  file(RELATIVE_PATH _rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_hdr}")
		  get_filename_component(_rel_dir "${_rel}" DIRECTORY)
		  file(MAKE_DIRECTORY "${_wrap_root}/${_rel_dir}")

		  # Absolute path to the real header (prevents wrapper->wrapper recursion)
		  set(_real_hdr "${_hdr}")

		  file(RELATIVE_PATH rel_path "${_wrap_root}/${_rel}" "${_hdr}")
		  file(TO_CMAKE_PATH "${_hdr}" _hdr_for_line)

		  set(INCLUDE_HEADER_PATH "${_wrap_root}/${_rel}")
		  file(WRITE  ${INCLUDE_HEADER_PATH} "// Auto wrapper for ${_rel}\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#pragma once\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#pragma push_macro(\"SIM_NAME\")\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#undef SIM_NAME\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#define SIM_NAME ${sim_name}\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#include <bitloop/core/project.h>\n")
		  file(APPEND "${INCLUDE_HEADER_PATH}" "#line 1 \"${_hdr_for_line}\"\n") # Add #line BEFORE including the real header
		  file(APPEND ${INCLUDE_HEADER_PATH} "#include \"${rel_path}\"\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#pragma pop_macro(\"SIM_NAME\")\n")
		endforeach()


		 # Expose wrapper include tree publicly; keep real headers private
         target_include_directories(${_TARGET}
             PUBLIC
                 $<BUILD_INTERFACE:${_wrap_root}>
                 #$<BUILD_INTERFACE:${BITLOOP_BUILD_DIR}>   # to make bitloop_simulations.h visible
                 $<INSTALL_INTERFACE:include>
             PRIVATE
                 ${CMAKE_CURRENT_SOURCE_DIR}
         )
	endif()

	# Add target /data (if provided)
	get_property(_data_dirs GLOBAL PROPERTY BITLOOP_DATA_DEPENDENCIES)
	if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
		list(APPEND _data_dirs "${CMAKE_CURRENT_SOURCE_DIR}/data")
	endif()
	set_property(GLOBAL PROPERTY BITLOOP_DATA_DEPENDENCIES ${_data_dirs})


	apply_common_settings(${_TARGET})

	if (CMAKE_CURRENT_SOURCE_DIR STREQUAL BL_ROOT_PROJECT)
		# Finalizing project tree
		file(APPEND "${AUTOGEN_SIM_INCLUDES}" "\nvoid initialize_simulations() {\n")
		file(APPEND "${AUTOGEN_SIM_INCLUDES}" "    using namespace BL;\n")

		get_property(_project_names GLOBAL PROPERTY BITLOOP_PROJECT_NAMES)
		foreach(project_name IN LISTS _project_names)
			file(APPEND "${AUTOGEN_SIM_INCLUDES}"
				"    ProjectBase::addProjectFactoryInfo( ProjectBase::createProjectFactoryInfo<${project_name}_Project>() );\n")
		endforeach()

		file(APPEND "${AUTOGEN_SIM_INCLUDES}" "}\n")


		msg(STATUS "")
		msg(STATUS "──────── Merged Data Tree ────────")
		get_property(_data_dirs GLOBAL PROPERTY BITLOOP_DATA_DEPENDENCIES)

		get_filename_component(BITLOOP_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

		msg(STATUS "${BITLOOP_COMMON}/data")

		add_custom_command(TARGET ${_TARGET} PRE_BUILD 
			COMMAND ${CMAKE_COMMAND} -E copy_directory 
				"${BITLOOP_COMMON}/data"
				"$<TARGET_FILE_DIR:${_TARGET}>/data"
			COMMENT "Merging dependency data from ${dep_path}"
		)

		foreach(dep_path IN LISTS _data_dirs)
			file(RELATIVE_PATH rel_src "${BITLOOP_PARENT_DIR}" "${dep_path}")
			file(RELATIVE_PATH rel_dest "${BITLOOP_PARENT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

			msg(STATUS "${rel_src}")

			add_custom_command(TARGET ${_TARGET} PRE_BUILD 
				COMMAND ${CMAKE_COMMAND} -E copy_directory 
					"${dep_path}" 
					"$<TARGET_FILE_DIR:${_TARGET}>/data"
				COMMENT "Merging dependency data from ${dep_path}"
			)
		endforeach()
		msg(STATUS "")

		apply_main_settings(${_TARGET})
	endif()

endmacro()

# Queues dependency path to be added when bitloop_new_project gets called
macro(bitloop_add_dependency DEP_PATH)
	get_filename_component(_SIM_DIR "${DEP_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_property(_list DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_DEPENDENCY_DIRS)
	list(APPEND _list ${_SIM_DIR})
	set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_DEPENDENCY_DIRS "${_list}")
endmacro()

# Actually includes/links the dependency into the target (internal)
function(_bitloop_add_dependency _TARGET _SIM_DIR)
	# Grab simulation name from path
	get_filename_component(sim_name "${_SIM_DIR}" NAME)

	if(NOT TARGET ${sim_name}::${sim_name})
		# Only called once, includes dependency project, which in turn calls:  bitloop_add_simulation()
		add_subdirectory(${_SIM_DIR} "${CMAKE_BINARY_DIR}/${sim_name}_build")
	endif()


	# Link dependency into our target
	target_link_libraries(${_TARGET} PUBLIC ${sim_name}::${sim_name})
endfunction()


# --- msg() helper with indentation push/pop support ---
set_property(GLOBAL PROPERTY MSG_INDENT_LEVEL 0)
function(msg_indent_push)
  get_property(_lvl GLOBAL PROPERTY MSG_INDENT_LEVEL)
  math(EXPR _lvl "${_lvl} + 1")
  set_property(GLOBAL PROPERTY MSG_INDENT_LEVEL "${_lvl}")
endfunction()
function(msg_indent_pop)
  get_property(_lvl GLOBAL PROPERTY MSG_INDENT_LEVEL)
  math(EXPR _lvl "${_lvl} - 1")
  if(_lvl LESS 0)  
    set(_lvl 0)
  endif()
  set_property(GLOBAL PROPERTY MSG_INDENT_LEVEL "${_lvl}")
endfunction()
function(msg)
  set(_status ${ARGV0})
  list(LENGTH ARGV _len)
  math(EXPR _count "${_len} - 1")
  if(_count GREATER 0)
    list(SUBLIST ARGV 1 ${_count} _parts)
    string(JOIN " " _text ${_parts})
  else()
    set(_text "")
  endif()
  get_property(_lvl GLOBAL PROPERTY MSG_INDENT_LEVEL)
  if(_lvl LESS 1)
    set(_indent "")
  else()
    set(_indent "")
    foreach(_i RANGE 1 ${_lvl})
      set(_indent "${_indent}   ")
    endforeach()
  endif()
  message(${_status} "${_indent}${_text}")
endfunction()

