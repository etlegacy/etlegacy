#-----------------------------------------------------------------
# Program
#-----------------------------------------------------------------

# Check required executables
if(FEATURE_RENDERER2)
	# Find GNU sed executable, BSD variant does not work with our script
	if(UNIX AND NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
		find_program(SED_EXECUTABLE gsed)
	else()
		find_program(SED_EXECUTABLE sed)
	endif()
endif(FEATURE_RENDERER2)
