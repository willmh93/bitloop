# --- bitloopSimulation.cmake ---

set(BL_SHARE		"${CMAKE_CURRENT_LIST_DIR}/..")
set(BL_MAIN_SOURCE	"${BL_SHARE}/src/bitloop_main.cpp"	CACHE INTERNAL "")
set(BL_COMMON		"${BL_SHARE}/common"				CACHE INTERNAL "")

set(BL_PROJECTS				""	CACHE INTERNAL "included projected")
set(BL_DATA_DEPENDENCIES	""	CACHE INTERNAL "Ordered list of dependency data directories")



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
			-O3
			-sUSE_PTHREADS=1
			-pthread
			-matomics
			-mbulk-memory

			$<$<CONFIG:RelWithDebInfo>:-g2> # keep DWARF + names in release
			$<$<CONFIG:RelWithDebInfo>:-ffunction-sections>
			$<$<CONFIG:RelWithDebInfo>:-fdata-sections>
		)

		target_link_options(${_TARGET} PRIVATE
			-sUSE_SDL=3
			-sUSE_WEBGL2=1
			-sFULL_ES3=1
			-sALLOW_MEMORY_GROWTH=1
			-sUSE_PTHREADS=1
			-sPTHREAD_POOL_SIZE_STRICT=0
			-sPTHREAD_POOL_SIZE=32
			-sPTHREADS_DEBUG=1
			-sEXPORTED_RUNTIME_METHODS=[ccall,UTF8ToString,stringToUTF8,lengthBytesUTF8]

			# Prefer this OFF in Release (it bloats logs/binary)
			-sPTHREADS_DEBUG=0

			# Names + demangling for analysis
			$<$<CONFIG:RelWithDebInfo>:--profiling-funcs>

			# GC unreferenced sections
			$<$<CONFIG:RelWithDebInfo>:-Wl>
			$<$<CONFIG:RelWithDebInfo>:--gc-sections>
			$<$<CONFIG:RelWithDebInfo>:-flto>
		)
	endif()
endfunction()

function(_apply_main_settings _TARGET)
	if (MSVC)
		set_target_properties(${_TARGET} PROPERTIES WIN32_EXECUTABLE TRUE)
	elseif (EMSCRIPTEN)
		target_link_options(${_TARGET} PRIVATE
			"--shell-file=${BL_COMMON}/static/shell.html"
			#"--embed-file=${CMAKE_CURRENT_BINARY_DIR}/data@/data"
			"--embed-file=${CMAKE_SOURCE_DIR}/build/${BUILD_FLAVOR}/app/data@/data"
		)
		set_target_properties(${_TARGET} PROPERTIES
			OUTPUT_NAME "index"
			SUFFIX ".html"
		)
		_optimize_wasm(${_TARGET})
	endif()
endfunction()

function(_optimize_wasm _TARGET)
	# _HOST_TRIPLET: prefer what vcpkg gives us; otherwise infer from host OS/arch
	set(_HOST_TRIPLET "${VCPKG_HOST_TRIPLET}")
	if(NOT _HOST_TRIPLET)
	  string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" _host_arch)
	  if(CMAKE_HOST_WIN32)
		if(_host_arch MATCHES "arm64|aarch64")
		  set(_HOST_TRIPLET "arm64-windows")
		elseif(_host_arch MATCHES "x86_64|amd64|x64")
		  set(_HOST_TRIPLET "x64-windows")
		else()
		  set(_HOST_TRIPLET "x86-windows")
		endif()
	  elseif(CMAKE_HOST_APPLE)
		if(_host_arch MATCHES "arm64|aarch64")
		  set(_HOST_TRIPLET "arm64-osx")
		else()
		  set(_HOST_TRIPLET "x64-osx")
		endif()
	  elseif(CMAKE_HOST_UNIX)
		if(_host_arch MATCHES "arm64|aarch64")
		  set(_HOST_TRIPLET "arm64-linux")
		else()
		  set(_HOST_TRIPLET "x64-linux")
		endif()
	  else()
		# last-ditch default
		set(_HOST_TRIPLET "x64-windows")
	  endif()
	endif()

	message(STATUS "_HOST_TRIPLET: ${_HOST_TRIPLET}")
	message(STATUS "BROTLI HINT: ${CMAKE_BINARY_DIR}/vcpkg_installed/${_HOST_TRIPLET}/tools/brotli-tool")

	# where vcpkg puts host tools
	set(_BROTLI_HINTS "${CMAKE_BINARY_DIR}/vcpkg_installed/${_HOST_TRIPLET}/tools/brotli-tool")
	find_program(BROTLI_EXECUTABLE NAMES brotli brotli.exe HINTS ${_BROTLI_HINTS})

	# Compute wasm paths using genex (resolved at build time)
	set(_WASM "$<TARGET_FILE_DIR:${_TARGET}>/$<TARGET_FILE_BASE_NAME:${_TARGET}>.wasm")

	# Precompress with brotli
	message(STATUS "BROTLI_EXECUTABLE: ${BROTLI_EXECUTABLE}")
	add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND "${BROTLI_EXECUTABLE}" -f -q 11 "${_WASM}" VERBATIM)
  
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
	#message(STATUS "Setting ${name} visibility: " ${value})
	set_property(GLOBAL PROPERTY "BL_PROJECT_VISIBILITY_${name}" "${value}")

	get_property(_is_set GLOBAL PROPERTY "BL_PROJECT_VISIBILITY_${name}" SET)

	if (NOT _is_set)
		message(FATAL_ERROR "Failed to set visibility for project ${name}")
	endif()
endfunction()

function(_bitloop_get_project_visibility name out_var)
	# Is the property set?
	get_property(_is_set GLOBAL PROPERTY "BL_PROJECT_VISIBILITY_${name}" SET)
	if (_is_set)
		get_property(_v GLOBAL PROPERTY "BL_PROJECT_VISIBILITY_${name}")
	else()
		set(_v ON)  # default
	endif()
	set(${out_var} "${_v}" PARENT_SCOPE)
endfunction()

# ──────────────────────────────────────────────────────────
# ───────────────────────── Public ─────────────────────────
# ──────────────────────────────────────────────────────────

function(bitloop_new_project sim_name)
	
	# collect the other args as source files
	set(SIM_SOURCES ${ARGN})

	
	message(STATUS "bitloop_new_project(${sim_name}) -- ${CMAKE_CURRENT_SOURCE_DIR}")

	# First new_project call? Set as "root" since queued children are guaranteed to be processed later
	get_property(BL_ROOT_PROJECT GLOBAL PROPERTY BL_ROOT_PROJECT)
	if (NOT BL_ROOT_PROJECT)
		set_property(GLOBAL PROPERTY BL_ROOT_PROJECT ${CMAKE_CURRENT_SOURCE_DIR})
		set(BL_ROOT_PROJECT ${CMAKE_CURRENT_SOURCE_DIR})
		set(BL_AUTOGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/include_autogen" CACHE INTERNAL "")
	endif()

	if (CMAKE_CURRENT_SOURCE_DIR STREQUAL BL_ROOT_PROJECT)
		set(IS_ROOT_PROJECT TRUE)
	else()
		set(IS_ROOT_PROJECT FALSE)
	endif()

	set(_TARGET ${sim_name})

	if (IS_ROOT_PROJECT)
		# ────────── top-level (executable) ──────────
		message(STATUS "ROOT EXECUTABLE: ${_TARGET}")

		if(NOT SIM_SOURCES)
			set(SIM_SOURCES ${BL_MAIN_SOURCE}) # no user sources (e.g. superbuild), just use the framework main()
			set(SIM_SOURCES_PROVIDED FALSE)
		else()
			list(APPEND SIM_SOURCES ${BL_MAIN_SOURCE}) # sources provided by user, so append the framework main()
			set(SIM_SOURCES_PROVIDED TRUE)
		endif()

		set(_type STATIC)

		set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${_TARGET}")
		add_executable(${_TARGET} ${SIM_SOURCES})
		target_include_directories(${_TARGET} PRIVATE "$<BUILD_INTERFACE:${BL_AUTOGEN_DIR}>")

		_apply_root_exe_name(${_TARGET})

	else()
		# ────────── nested (library) ──────────
		set(SIM_SOURCES_PROVIDED TRUE)

		if(NOT SIM_SOURCES)
			set(SIM_SOURCES_PROVIDED FALSE)
			set(_type INTERFACE)
		else()
			set(SIM_SOURCES_PROVIDED TRUE)
			set(_type STATIC)
		endif()

		if (NOT TARGET ${_TARGET})
			add_library(${_TARGET} ${_type} ${SIM_SOURCES})
		endif()

		message(STATUS "add_library(${sim_name}::${sim_name})")
		add_library(${sim_name}::${sim_name} ALIAS ${sim_name}) # Export an alias so consumers can link as sim::sim
	endif()

	message(STATUS "${sim_name} IS ${_type}")

	set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY _TYPE ${_type})
	set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY BL_TARGET_PROJECT ${_TARGET})

	# Link exe/lib to bitloop
	if (NOT _type STREQUAL "INTERFACE")

		message(STATUS "LINKING ${_TARGET} to bitloop::bitloop")
		target_link_libraries(${_TARGET} PRIVATE bitloop::bitloop)

		# Add project names to list in order of inclusion
		if (SIM_SOURCES_PROVIDED)
			get_property(_project_names GLOBAL PROPERTY BL_PROJECTS)
			if (NOT sim_name IN_LIST _project_names)
				list(APPEND _project_names ${sim_name})
			endif()
			set_property(GLOBAL PROPERTY BL_PROJECTS	${_project_names})
		endif()

		# Write include wrapper
		set(_shim "${CMAKE_CURRENT_BINARY_DIR}/.bl_project_metadata.h")
		file(WRITE "${_shim}" "#pragma once\n#define SIM_NAME ${sim_name}\n")
        
		if (MSVC)
			target_compile_options(${_TARGET} PRIVATE "/FI${_shim}")
		else()
			target_compile_options(${_TARGET} PRIVATE "-include" "${_shim}")
		endif()
	endif()

	# If root project, start printing project tree digram
	if (IS_ROOT_PROJECT)
		message(STATUS "")
		message(STATUS "────────── Project Tree ──────────")
		message(STATUS "[${sim_name}]")
	else()
		msg(STATUS "[${sim_name}]")
	endif()

	if (NOT _type STREQUAL "INTERFACE")
		#if (SIM_SOURCES_PROVIDED)
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

			# Add target "/data" (if provided)
			get_property(_data_dirs GLOBAL PROPERTY BL_DATA_DEPENDENCIES)
			if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
				list(APPEND _data_dirs "${CMAKE_CURRENT_SOURCE_DIR}/data")
			endif()
			set_property(GLOBAL PROPERTY BL_DATA_DEPENDENCIES ${_data_dirs})

			# Apply settings for all projects
			_apply_common_settings(${_TARGET})
		#endif()
	endif()
endfunction()

function(bitloop_add_dependency _TARGET _SIM_DIR)
	message(STATUS "bitloop_add_dependency(${_TARGET}, ${_SIM_DIR}) - ${CMAKE_CURRENT_SOURCE_DIR}")

	get_property(_type DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY _TYPE)
	message(STATUS "_type: ${_type}")

	# Grab simulation name from path
	get_filename_component(sim_name "${_SIM_DIR}" NAME)

	if(NOT TARGET ${sim_name}::${sim_name})
		# Only called once, includes dependency project, which in turn calls:  bitloop_add_project()
		msg_indent_push()
		add_subdirectory(${_SIM_DIR} "${CMAKE_BINARY_DIR}/${sim_name}_build")
		msg_indent_pop()
	endif()

	# Link dependency into our target
	if (_type STREQUAL "INTERFACE")
		message(STATUS "DETECTED INTERFACE")
		target_link_libraries(${_TARGET} INTERFACE ${sim_name}::${sim_name})
	else()
		message(STATUS "DETECTED STATIC")
		target_link_libraries(${_TARGET} PUBLIC ${sim_name}::${sim_name})
	endif()
endfunction()

# Auto-generated finalization step; adds includes to autogen header and bundles data into executable
macro(bitloop_finalize)
	get_property(_TARGET DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY BL_TARGET_PROJECT)
	message(STATUS "${_TARGET} Finalizing (${CMAKE_CURRENT_SOURCE_DIR})")

	if(PROJECT_IS_TOP_LEVEL)
		message(STATUS "${_TARGET} is top-level (${CMAKE_CURRENT_SOURCE_DIR})")
	else()
		message(STATUS "${_TARGET} is NOT top-level (${CMAKE_CURRENT_SOURCE_DIR})")
	endif()

	get_property(BL_ROOT_PROJECT GLOBAL PROPERTY BL_ROOT_PROJECT)
	if (CMAKE_CURRENT_SOURCE_DIR STREQUAL BL_ROOT_PROJECT)
		#message(STATUS "")
		#message(STATUS "──────── Finalizing ────────")
		#get_property(_global_projects GLOBAL PROPERTY BL_PROJECTS)
		#foreach(project_name IN LISTS _global_projects)
		#	msg(STATUS "<<<${project_name}>>>")
		#endforeach()

		# Begin auto-generated header file
		
		file(MAKE_DIRECTORY			"${BL_AUTOGEN_DIR}")
		set(BL_AUTOGEN_INCLUDES		"${BL_AUTOGEN_DIR}/bitloop_simulations.h")
		message(STATUS "Creating: ${BL_AUTOGEN_DIR}/bitloop_simulations.h")

		file(WRITE "${BL_AUTOGEN_INCLUDES}" "// Auto‑generated simulation includes\n")
		file(APPEND "${BL_AUTOGEN_INCLUDES}" "#include <bitloop/core/project.h>\n\n")

		get_property(_project_names GLOBAL PROPERTY BL_PROJECTS)

		####################################################
		#####  bitloop_simulation.h - project includes #####
		####################################################

		foreach(project_name IN LISTS _project_names)
			# Append #include to the auto-generated "bitloop_simulation.h" header
			file(APPEND "${BL_AUTOGEN_INCLUDES}" "#pragma push_macro(\"SIM_NAME\")\n")
			file(APPEND "${BL_AUTOGEN_INCLUDES}" "#undef SIM_NAME\n")
			file(APPEND "${BL_AUTOGEN_INCLUDES}" "#define SIM_NAME ${project_name}\n")
			file(APPEND "${BL_AUTOGEN_INCLUDES}" "#include <${project_name}/${project_name}.h>\n")
			file(APPEND "${BL_AUTOGEN_INCLUDES}" "using ${project_name}::${project_name}_Project;\n")
			file(APPEND "${BL_AUTOGEN_INCLUDES}" "#pragma pop_macro(\"SIM_NAME\")\n\n")
		endforeach()

		#####################################################
		#####  bitloop_simulation.h - project factories #####
		#####################################################

		file(APPEND "${BL_AUTOGEN_INCLUDES}" "\nvoid initialize_simulations() {\n")
		file(APPEND "${BL_AUTOGEN_INCLUDES}" "    using namespace bl;\n")

		foreach(project_name IN LISTS _project_names)
			_bitloop_get_project_visibility(${project_name} project_visible)
			if (project_visible)
				file(APPEND "${BL_AUTOGEN_INCLUDES}"
					"    ProjectBase::addProjectFactoryInfo( ProjectBase::createProjectFactoryInfo<${project_name}_Project>(\"${project_name}\"));\n")
			endif()
		endforeach()
		file(APPEND "${BL_AUTOGEN_INCLUDES}" "}\n")

		#######################################
		##### Bundle data into executable #####
		#######################################

		message(STATUS "")
		message(STATUS "──────── Merged Data Tree ────────")
		get_property(_data_dirs GLOBAL PROPERTY BL_DATA_DEPENDENCIES)

		get_filename_component(BITLOOP_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

		# Add common data
		message(STATUS "${BL_COMMON}/data")
		add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory 
				"${BL_COMMON}/data"
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
	endif()
endmacro()

macro(bitloop_hide _TARGET)
	_bitloop_set_visibility(${_TARGET} OFF)
endmacro()

macro(bitloop_show _TARGET)
	_bitloop_set_visibility(${_TARGET} ON)
endmacro()


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

