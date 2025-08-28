# --- bitloopSimulation.cmake ---

set(BITLOOP_MAIN_SOURCE		"${CMAKE_CURRENT_LIST_DIR}/src/bitloop_main.cpp"	CACHE INTERNAL "")
set(BITLOOP_COMMON			"${CMAKE_CURRENT_LIST_DIR}/common"					CACHE INTERNAL "")

# Auto-generated include folder
set(BITLOOP_AUTOGEN_DIR		"${CMAKE_CURRENT_BINARY_DIR}/include_autogen" CACHE INTERNAL "")
file(MAKE_DIRECTORY			"${BITLOOP_AUTOGEN_DIR}")
set(AUTOGEN_SIM_INCLUDES	"${BITLOOP_AUTOGEN_DIR}/bitloop_simulations.h" CACHE INTERNAL "")

set(BITLOOP_PROJECT_NAMES		""		CACHE INTERNAL "Ordered included projected")
set(BITLOOP_PROJECT_VISIBILITY	""		CACHE INTERNAL "Ordered included projected visibility")
set(BITLOOP_DATA_DEPENDENCIES	""		CACHE INTERNAL "Ordered list of dependency data directories")

set_property(DIRECTORY PROPERTY BITLOOP_LOCAL_PROJECTS "")
set_property(DIRECTORY PROPERTY BITLOOP_DEPENDENCY_DIRS "")
set_property(DIRECTORY PROPERTY BITLOOP_ROOT_TARGET "")
set_property(DIRECTORY PROPERTY BITLOOP_ROOT_SOURCES "")

# Begin auto-generated header file
file(WRITE "${AUTOGEN_SIM_INCLUDES}" "// Auto‑generated simulation includes\n")
file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#include <bitloop/core/project.h>\n\n")


# ──────────────────────────────────────────────────────────
# ──────────────────────── Private ─────────────────────────
# ──────────────────────────────────────────────────────────

function(_apply_common_settings _TARGET)
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
			#"-pthread"
			"-sUSE_SDL=3"
			"-sUSE_WEBGL2=1"
			"-sFULL_ES3=1"
			"-sALLOW_MEMORY_GROWTH=1"
			"-sUSE_PTHREADS=1"
			"-sPTHREAD_POOL_SIZE_STRICT=0"
			"-sPTHREAD_POOL_SIZE=32"
			"-sPTHREADS_DEBUG=1"
			"-sEXPORTED_RUNTIME_METHODS=[ccall,UTF8ToString,stringToUTF8,lengthBytesUTF8]"
		)
	endif()
endfunction()

function(_apply_main_settings _TARGET)
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

function(_apply_root_exe_name target)
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

function(_bitloop_set_visibility name value)
	get_property(_is_set GLOBAL PROPERTY "BITLOOP_PROJECT_VISIBILITY_${name}" SET)
	if (_is_set)
		# Parent must have already set visibility (which takes priority)
		return()
	endif()
	message(STATUS "Setting ${name} visibility: " ${value})
	set_property(GLOBAL PROPERTY "BITLOOP_PROJECT_VISIBILITY_${name}" "${value}")
endfunction()

function(_bitloop_get_project_visibility name out_var)
	# Is the property set?
	get_property(_is_set GLOBAL PROPERTY "BITLOOP_PROJECT_VISIBILITY_${name}" SET)
	if (_is_set)
		get_property(_v GLOBAL PROPERTY "BITLOOP_PROJECT_VISIBILITY_${name}")
	else()
		set(_v ON)  # default
	endif()
	set(${out_var} "${_v}" PARENT_SCOPE)
endfunction()



function(_bitloop_new_project sim_name)
	#message(STATUS "${CMAKE_CURRENT_SOURCE_DIR} -> Adding project [${sim_name}]")

	# collect the other args as source files
	set(SIM_SOURCES ${ARGN})

	# First new_project call? Set as "root" since queued children are guaranteed to be processed later
	if (NOT BL_ROOT_PROJECT)
		set(BL_ROOT_PROJECT ${CMAKE_CURRENT_SOURCE_DIR})
	endif()

	if (CMAKE_CURRENT_SOURCE_DIR STREQUAL BL_ROOT_PROJECT)
		set(IS_ROOT_PROJECT TRUE)
	else()
		set(IS_ROOT_PROJECT FALSE)
	endif()

	if (IS_ROOT_PROJECT)
		##################################
		##### top-level (executable) #####
		##################################
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
		target_include_directories(${_TARGET} PRIVATE "$<BUILD_INTERFACE:${BITLOOP_AUTOGEN_DIR}>")

		_apply_root_exe_name(${_TARGET})

	else()
		#############################
		###### nested (library) #####
		#############################
		set(_TARGET ${sim_name})
		set(SIM_SOURCES_PROVIDED TRUE)

		add_library(${_TARGET} ${SIM_SOURCES})
		add_library(${sim_name}::${sim_name} ALIAS ${sim_name}) # Export an alias so consumers can link as sim::sim
	endif()

	# Link exe/lib to bitloop
	target_link_libraries(${_TARGET} PRIVATE bitloop::bitloop)

	# If real project (not just for organization)
	if (SIM_SOURCES_PROVIDED)
		# Append #include to the auto-generated "bitloop_simulation.h" header
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#pragma push_macro(\"SIM_NAME\")\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#undef SIM_NAME\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#define SIM_NAME ${sim_name}\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#include <${sim_name}/${sim_name}.h>\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "using ${sim_name}::${sim_name}_Project;\n")
        file(APPEND "${AUTOGEN_SIM_INCLUDES}" "#pragma pop_macro(\"SIM_NAME\")\n\n")

		# Write include wrapper
        set(_shim "${CMAKE_CURRENT_BINARY_DIR}/.bl_project_metadata.h")
        file(WRITE "${_shim}" "#pragma once\n#define SIM_NAME ${sim_name}\n")
        
        if (MSVC)
          target_compile_options(${_TARGET} PRIVATE "/FI${_shim}")
        else()
          target_compile_options(${_TARGET} PRIVATE "-include" "${_shim}")
        endif()
	endif()

	# Add project names to list in order of inclusion
	if (SIM_SOURCES_PROVIDED)
		get_property(_project_names GLOBAL PROPERTY BITLOOP_PROJECT_NAMES)
		if (NOT sim_name IN_LIST _project_names)
			list(APPEND _project_names ${sim_name})
		endif()
		set_property(GLOBAL								  PROPERTY BITLOOP_PROJECT_NAMES	${_project_names})
	endif()

	# If root project, start printing project tree digram
	if (IS_ROOT_PROJECT)
		message(STATUS "")
		message(STATUS "────────── Project Tree ──────────")
		message(STATUS "[${sim_name}]")
	endif()

	

	# With each level of recursion, indentation increases
	msg_indent_push()
	get_property(_list DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_DEPENDENCY_DIRS)
	foreach(dep_path IN LISTS _list)
		get_filename_component(dep_name ${dep_path} NAME)

		# Print project name (with indentation)
		msg(STATUS "[${dep_name}]")

		# Process dependency project, which in turn calls _bitloop_new_project (indirect recursion)
		_bitloop_add_dependency(${_TARGET} ${dep_path})
	endforeach()
	msg_indent_pop()

	

	if (SIM_SOURCES_PROVIDED)
		# Include tree (include wrappers that define project namespace macros)
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

		  # Make full wrapper include dir
		  get_filename_component(_rel_dir "${_rel}" DIRECTORY)
		  file(MAKE_DIRECTORY "${_wrap_root}/${_rel_dir}")

		  # Relative include
		  file(RELATIVE_PATH rel_path "${_wrap_root}/${_rel}" "${_hdr}")
		  file(TO_CMAKE_PATH "${_hdr}" _hdr_for_line)

		  set(INCLUDE_HEADER_PATH "${_wrap_root}/${_rel}")
		  file(WRITE  ${INCLUDE_HEADER_PATH} "// Auto wrapper for ${_rel}\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#pragma once\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#pragma push_macro(\"SIM_NAME\")\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#undef SIM_NAME\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#define SIM_NAME ${sim_name}\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#include <bitloop/core/project.h>\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#line 1 \"${_hdr_for_line}\"\n") # Add #line BEFORE including the real header
		  file(APPEND ${INCLUDE_HEADER_PATH} "#include \"${rel_path}\"\n")
		  file(APPEND ${INCLUDE_HEADER_PATH} "#pragma pop_macro(\"SIM_NAME\")\n")
		endforeach()


		 # Expose wrapper include tree publicly; keep real headers private
         target_include_directories(${_TARGET}
             PUBLIC
                 $<BUILD_INTERFACE:${_wrap_root}>
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

	# Apply settings for all projects
	_apply_common_settings(${_TARGET})

	# Finalize everything
	if (IS_ROOT_PROJECT)
		_finalize_project_tree()
	endif()
endfunction()

function(_finalize_project_tree)

	message(STATUS "")
	message(STATUS "──────── Finalizing ────────")
	get_property(_global_projects GLOBAL PROPERTY BITLOOP_PROJECT_NAMES)
	foreach(project_name IN LISTS _global_projects)
		msg(STATUS "<<<${project_name}>>>")
	endforeach()

	##################################
	#####  add project factories #####
	##################################
	file(APPEND "${AUTOGEN_SIM_INCLUDES}" "\nvoid initialize_simulations() {\n")
	file(APPEND "${AUTOGEN_SIM_INCLUDES}" "    using namespace BL;\n")


	get_property(_project_names GLOBAL PROPERTY BITLOOP_PROJECT_NAMES)
	foreach(project_name IN LISTS _project_names)
		_bitloop_get_project_visibility(${project_name} project_visible)
		message(STATUS "Getting ${project_name} visibility: " ${project_visible})
		
		# Convert ON/OFF -> true/false for C++
		#if (project_visible)
		#	set(_visible_cpp true)
		#else()
		#	set(_visible_cpp false)
		#endif()
		
		if (project_visible)
			file(APPEND "${AUTOGEN_SIM_INCLUDES}"
			"    ProjectBase::addProjectFactoryInfo( ProjectBase::createProjectFactoryInfo<${project_name}_Project>());\n")
			#"    ProjectBase::addProjectFactoryInfo( ProjectBase::createProjectFactoryInfo<${project_name}_Project>(), ${_visible_cpp} );\n")
		endif()
	endforeach()
	file(APPEND "${AUTOGEN_SIM_INCLUDES}" "}\n")

	#######################################
	##### Bundle data into executable #####
	#######################################
	message(STATUS "")
	message(STATUS "──────── Merged Data Tree ────────")
	get_property(_data_dirs GLOBAL PROPERTY BITLOOP_DATA_DEPENDENCIES)

	get_filename_component(BITLOOP_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)


	# Add common data
	message(STATUS "${BITLOOP_COMMON}/data")
	add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory 
			"${BITLOOP_COMMON}/data"
			"$<TARGET_FILE_DIR:${_TARGET}>/data"
			COMMENT "Merging dependency data from ${dep_path}")

	# Merge in project-specific data
	foreach(dep_path IN LISTS _data_dirs)
		file(RELATIVE_PATH rel_src "${BITLOOP_PARENT_DIR}" "${dep_path}")
		file(RELATIVE_PATH rel_dest "${BITLOOP_PARENT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
		message(STATUS "${rel_src}")

		add_custom_command(TARGET ${_TARGET} PRE_BUILD 
			COMMAND ${CMAKE_COMMAND} -E copy_directory 
				"${dep_path}" 
				"$<TARGET_FILE_DIR:${_TARGET}>/data"
				COMMENT "Merging dependency data from ${dep_path}")
	endforeach()
	message(STATUS "")

	# Apply settings for just the root project
	_apply_main_settings(${_TARGET})
endfunction()

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


# ──────────────────────────────────────────────────────────
# ───────────────────────── Public ─────────────────────────
# ──────────────────────────────────────────────────────────

# Queues dependency path to be added when bitloop_new_project gets called
macro(bitloop_add_dependency DEP_PATH)
	get_filename_component(_SIM_DIR "${DEP_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_property(_list DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_DEPENDENCY_DIRS)
	list(APPEND _list ${_SIM_DIR})
	set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_DEPENDENCY_DIRS "${_list}")

	get_filename_component(sim_name "${DEP_PATH}" NAME)
	get_property(_local_projects DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_LOCAL_PROJECTS)
	if (NOT sim_name IN_LIST _project_names)
		list(APPEND _local_projects ${sim_name})
	endif()
	set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}  PROPERTY BITLOOP_LOCAL_PROJECTS  ${_local_projects})
endmacro()

# Queues root project to be added when bitloop_finalize() gets called
macro(bitloop_new_project ROOT_TARGET)
	set(_args ${ARGV})
	list(REMOVE_AT _args 0)

	set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_ROOT_TARGET  "${ROOT_TARGET}")
	set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_ROOT_SOURCES "${_args}")


	get_property(_local_projects DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_LOCAL_PROJECTS)
	if (NOT ROOT_TARGET IN_LIST _project_names)
		list(APPEND _local_projects ${ROOT_TARGET})
	endif()
	set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}  PROPERTY BITLOOP_LOCAL_PROJECTS  ${_local_projects})
endmacro()

# Processes the queued root project / dependencies
macro(bitloop_finalize)
	get_property(_root_target DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_ROOT_TARGET)
	get_property(_root_sources DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_ROOT_SOURCES)
	if(NOT _root_target)
		message(FATAL_ERROR "bitloop_finalize(): no root project queued. Call bitloop_add_project() first, or create the target before finalize.")
	endif()
		
	_bitloop_new_project(${_root_target} ${_root_sources})
endmacro()

macro(bitloop_hide _TARGET)
	_bitloop_set_visibility(${_TARGET} OFF)
endmacro()

macro(bitloop_show _TARGET)
	_bitloop_set_visibility(${_TARGET} ON)
endmacro()

#macro(bitloop_show_only_tree _TARGET)
#	
#	message(STATUS "Showing ONLY: ${_TARGET}")
#	get_property(_local_projects DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BITLOOP_LOCAL_PROJECTS)
#
#	foreach(project_name IN LISTS _local_projects)
#		message(STATUS "Hiding: ${project_name}")
#		bitloop_hide(${project_name})
#	endforeach()
#
#	bitloop_show(${_TARGET})
#endmacro()


# ──────────────────────────────────────────────────────────
# ───────────────────────── Helper ─────────────────────────
# ──────────────────────────────────────────────────────────

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

