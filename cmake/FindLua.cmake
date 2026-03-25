# Distributed under the OSI-approved BSD 3-Clause License.	See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.
# https://github.com/Kitware/CMake/blob/master/Modules/FindLua.cmake

cmake_policy(PUSH)	# Policies apply to functions at definition-time
cmake_policy(SET CMP0140 NEW)
cmake_policy(SET CMP0159 NEW)  # file(STRINGS) with REGEX updates CMAKE_MATCH_<n>

unset(_lua_include_subdirs)
unset(_lua_library_names)
unset(_lua_append_versions)

# this is a function only to have all the variables inside go away automatically
function(_lua_get_versions)
		set(LUA_VERSIONS5 5.5 5.4 5.3 5.2 5.1 5.0)

		if (Lua_FIND_VERSION_EXACT)
				if (Lua_FIND_VERSION_COUNT GREATER 1)
						set(_lua_append_versions ${Lua_FIND_VERSION_MAJOR}.${Lua_FIND_VERSION_MINOR})
				endif ()
		elseif (Lua_FIND_VERSION)
				# once there is a different major version supported this should become a loop
				if (NOT Lua_FIND_VERSION_MAJOR GREATER 5)
						if (Lua_FIND_VERSION_COUNT EQUAL 1)
								set(_lua_append_versions ${LUA_VERSIONS5})
						else ()
								foreach (subver IN LISTS LUA_VERSIONS5)
										if (NOT subver VERSION_LESS ${Lua_FIND_VERSION})
												list(APPEND _lua_append_versions ${subver})
										endif ()
								endforeach ()
								# New version -> Search for it (heuristic only! Defines in include might have changed)
								if (NOT _lua_append_versions)
										set(_lua_append_versions ${Lua_FIND_VERSION_MAJOR}.${Lua_FIND_VERSION_MINOR})
								endif()
						endif ()
				endif ()
		else ()
				# once there is a different major version supported this should become a loop
				set(_lua_append_versions ${LUA_VERSIONS5})
		endif ()

		if (LUA_Debug)
				message(STATUS "Considering following Lua versions: ${_lua_append_versions}")
		endif()

		set(_lua_append_versions "${_lua_append_versions}" PARENT_SCOPE)
endfunction()

function(_lua_set_version_vars)
	set(_lua_include_subdirs_raw "lua")

	foreach (ver IN LISTS _lua_append_versions)
		string(REGEX MATCH "^([0-9]+)\\.([0-9]+)$" _ver "${ver}")
		list(APPEND _lua_include_subdirs_raw
				lua${CMAKE_MATCH_1}${CMAKE_MATCH_2}
				lua${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
				lua-${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
				)
	endforeach ()

	# Prepend "include/" to each path directly after the path
	set(_lua_include_subdirs "include")
	foreach (dir IN LISTS _lua_include_subdirs_raw)
		list(APPEND _lua_include_subdirs "${dir}" "include/${dir}")
	endforeach ()

	set(_lua_include_subdirs "${_lua_include_subdirs}" PARENT_SCOPE)
endfunction()

function(_lua_get_header_version)
	unset(Lua_VERSION PARENT_SCOPE)
	set(_hdr_file "${LUA_INCLUDE_DIR}/lua.h")

	if (NOT EXISTS "${_hdr_file}")
		return()
	endif ()

	# At least 5.[012] have different ways to express the version
	# so all of them need to be tested. Lua 5.2 defines LUA_VERSION
	# and LUA_RELEASE as joined by the C preprocessor, so avoid those.
	file(STRINGS "${_hdr_file}" lua_version_strings
			 REGEX "^#define[ \t]+LUA_(RELEASE[ \t]+\"Lua [0-9]|VERSION([ \t]+\"Lua [0-9]|_[MR])).*")

	string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_MAJOR(_N)?[ \t]+\"?([0-9])\"?[ \t]*;.*" "\\2" Lua_VERSION_MAJOR ";${lua_version_strings};")

	if (Lua_VERSION_MAJOR MATCHES "^[0-9]+$")
		string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_MINOR(_N)?[ \t]+\"?([0-9])\"?[ \t]*;.*" "\\2" Lua_VERSION_MINOR ";${lua_version_strings};")
		string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_RELEASE(_N)?[ \t]+\"?([0-9])\"?[ \t]*;.*" "\\2" Lua_VERSION_PATCH ";${lua_version_strings};")
		set(Lua_VERSION "${Lua_VERSION_MAJOR}.${Lua_VERSION_MINOR}.${Lua_VERSION_PATCH}")
	else ()
		string(REGEX REPLACE ".*;#define[ \t]+LUA_RELEASE[ \t]+\"Lua ([0-9.]+)\"[ \t]*;.*" "\\1" Lua_VERSION ";${lua_version_strings};")
		if (NOT Lua_VERSION MATCHES "^[0-9.]+$")
			string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION[ \t]+\"Lua ([0-9.]+)\"[ \t]*;.*" "\\1" Lua_VERSION ";${lua_version_strings};")
		endif ()
		string(REGEX REPLACE "^([0-9]+)\\.[0-9.]*$" "\\1" Lua_VERSION_MAJOR "${Lua_VERSION}")
		string(REGEX REPLACE "^[0-9]+\\.([0-9]+)[0-9.]*$" "\\1" Lua_VERSION_MINOR "${Lua_VERSION}")
		string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]).*" "\\1" Lua_VERSION_PATCH "${Lua_VERSION}")
	endif ()
	foreach (ver IN LISTS _lua_append_versions)
		if (ver STREQUAL "${Lua_VERSION_MAJOR}.${Lua_VERSION_MINOR}")
			set(LUA_VERSION_STRING "${Lua_VERSION}")
			set(LUA_VERSION_MAJOR "${Lua_VERSION_MAJOR}")
			set(LUA_VERSION_MINOR "${Lua_VERSION_MINOR}")
			set(LUA_VERSION_PATCH "${Lua_VERSION_PATCH}")

			return(
				PROPAGATE
					Lua_VERSION
					Lua_VERSION_MAJOR
					Lua_VERSION_MINOR
					Lua_VERSION_PATCH
					LUA_VERSION_STRING
					LUA_VERSION_MAJOR
					LUA_VERSION_MINOR
					LUA_VERSION_PATCH
			)
		endif ()
	endforeach ()
endfunction()

function(_lua_find_header)
	_lua_set_version_vars()

	# Initialize as local variable
	set(CMAKE_IGNORE_PATH ${CMAKE_IGNORE_PATH})
	while (TRUE)
		# Find the next header to test. Check each possible subdir in order
		# This prefers e.g. higher versions as they are earlier in the list
		# It is also consistent with previous versions of FindLua
		foreach (subdir IN LISTS _lua_include_subdirs)
			find_path(LUA_INCLUDE_DIR lua.h
				HINTS ENV LUA_DIR
				PATH_SUFFIXES ${subdir}
				)
			if (LUA_INCLUDE_DIR)
				break()
			endif()
		endforeach()
		# Did not found header -> Fail
		if (NOT LUA_INCLUDE_DIR)
			return()
		endif()
		_lua_get_header_version()
		# Found accepted version -> Ok
		if (Lua_VERSION)
			if (LUA_Debug)
				message(STATUS "Found suitable version ${Lua_VERSION} in ${LUA_INCLUDE_DIR}/lua.h")
			endif()
			return()
		endif()
		# Found wrong version -> Ignore this path and retry
		if (LUA_Debug)
			message(STATUS "Ignoring unsuitable version in ${LUA_INCLUDE_DIR}")
		endif()
		list(APPEND CMAKE_IGNORE_PATH "${LUA_INCLUDE_DIR}")
		unset(LUA_INCLUDE_DIR CACHE)
		unset(LUA_INCLUDE_DIR)
		unset(LUA_INCLUDE_DIR PARENT_SCOPE)
	endwhile ()
endfunction()

_lua_get_versions()
_lua_find_header()
_lua_get_header_version()
unset(_lua_append_versions)

if (Lua_VERSION)
	set(_lua_library_names
		lua${Lua_VERSION_MAJOR}${Lua_VERSION_MINOR}
		lua${Lua_VERSION_MAJOR}.${Lua_VERSION_MINOR}
		lua-${Lua_VERSION_MAJOR}.${Lua_VERSION_MINOR}
		lua.${Lua_VERSION_MAJOR}.${Lua_VERSION_MINOR}
		)
endif ()

find_library(LUA_LIBRARY
	NAMES ${_lua_library_names} lua
	NAMES_PER_DIR
	HINTS
		ENV LUA_DIR
	PATH_SUFFIXES lib
)
unset(_lua_library_names)

if (LUA_LIBRARY)
	# include the math library for Unix
	if (UNIX AND NOT APPLE AND NOT BEOS)
		find_library(LUA_MATH_LIBRARY m)
		mark_as_advanced(LUA_MATH_LIBRARY)
		set(LUA_LIBRARIES "${LUA_LIBRARY};${LUA_MATH_LIBRARY}")

		# include dl library for statically-linked Lua library
		get_filename_component(LUA_LIB_EXT ${LUA_LIBRARY} EXT)
		if(LUA_LIB_EXT STREQUAL CMAKE_STATIC_LIBRARY_SUFFIX)
			list(APPEND LUA_LIBRARIES ${CMAKE_DL_LIBS})
		endif()

	# For Windows and Mac, don't need to explicitly include the math library
	else ()
		set(LUA_LIBRARIES "${LUA_LIBRARY}")
	endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lua
																	REQUIRED_VARS LUA_LIBRARIES LUA_INCLUDE_DIR
																	VERSION_VAR Lua_VERSION)

mark_as_advanced(LUA_INCLUDE_DIR LUA_LIBRARY)

cmake_policy(POP)
