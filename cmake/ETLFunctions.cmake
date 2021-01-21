# General macros & functios used by the build process

function(LEG_BUNDLE _NAME _DESC)
	# Cannot use ARGN directly with list() command.
	# Copy to a variable first.
	set (extra_macro_args ${ARGN})
	set(LEG_BUNDLE_${_NAME}_VALID TRUE)

	# Did we get any optional args?
	list(LENGTH extra_macro_args num_extra_args)
	if (${num_extra_args} GREATER 0)
		foreach(argument ${extra_macro_args})
			# message(STATUS "Testing rule ${argument} for ${_DESC}")
			separate_arguments(argument)
			if(NOT (${argument}))
				# message(STATUS "${_DESC} failed")
				set(LEG_BUNDLE_${_NAME}_VALID FALSE)
			endif()
		endforeach()
	endif ()

	list(APPEND ALLOPTIONS "BUNDLED_${_NAME}")
	if (${LEG_BUNDLE_${_NAME}_VALID})
		cmake_dependent_option("BUNDLED_${_NAME}" "Use bundled ${_DESC} library instead of the system one." ${BUNDLED_LIBS_DEFAULT} "BUNDLED_LIBS" OFF)
	else()
		cmake_dependent_option("BUNDLED_${_NAME}" "Use bundled ${_DESC} library instead of the system one." OFF "BUNDLED_LIBS" OFF)
	endif ()
endfunction()

macro(LEG_EXTRACT _MSG _PATH _EXTRACT _EXTRACT_RES)
	message(STATUS "Extracting ${_MSG} to ${_EXTRACT_RES}")
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar -xzf ${_PATH}
		WORKING_DIRECTORY ${_EXTRACT}
	)
endmacro()

# We use a function here as we dont wont to contaminate the parent context with variables
# as you cannot send values outside a function without set(<variable> <value> PARENT_SCOPE)
# Usage: LEG_DOWNLOAD( <MESSAGE> <FILE URL> <DL TARGET> <HASH - OR BOOLEAN> <EXTRACT WORKING PATH> <EXTRACT EXPECTED RESULT> )
function(LEG_DOWNLOAD _MSG _URL _PATH _HASH _EXTRACT _EXTRACT_RES)
	set(DO_HASH "${_HASH}")
	set(ETLEGACY_DO_DOWNLOAD FALSE)

	if(NOT EXISTS "${_PATH}")
		message(STATUS "Downloading ${_MSG} to ${_PATH}")
		set(ETLEGACY_DO_DOWNLOAD TRUE)
	elseif(DO_HASH)
		if(DO_HASH STREQUAL TRUE)
			message(STATUS "Downloading ${_MSG} to ${_PATH}")
			set(ETLEGACY_DO_DOWNLOAD TRUE)
		else()
			file(MD5 "${_PATH}" ETLEGACY_DOWNLOAD_HASH)
			string(STRIP ${_HASH} test_hash)
			if(NOT (ETLEGACY_DOWNLOAD_HASH STREQUAL ${_HASH}))
				set(ETLEGACY_DO_DOWNLOAD TRUE)
				message(STATUS "${_MSG} hash does not match ( '${ETLEGACY_DOWNLOAD_HASH}' != '${_HASH}') . Re-downloading.")
			endif()
		endif()
	endif()

	if(ETLEGACY_DO_DOWNLOAD)
		file(DOWNLOAD
			${_URL}
			"${_PATH}"
			SHOW_PROGRESS TIMEOUT 30
		)

		if(_EXTRACT AND _EXTRACT_RES)
			LEG_EXTRACT("${_MSG}" "${_PATH}" "${_EXTRACT}" "${_EXTRACT_RES}")
		endif()
	else()
		if(_EXTRACT AND _EXTRACT_RES AND NOT EXISTS "${_EXTRACT_RES}")
			LEG_EXTRACT("${_MSG}" "${_PATH}" "${_EXTRACT}" "${_EXTRACT_RES}")
		endif()
	endif()
endfunction()
