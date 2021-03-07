#-----------------------------------------------------------------
# Bundled Libs
#-----------------------------------------------------------------

# Check if the libs submodule exists and add the directory
# or error out the build
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/CMakeLists.txt")
	message(STATUS "Using bundled libraries located at ${CMAKE_SOURCE_DIR}/libs")
	if(NOT ANDROID)
		include(libs/CMakeLists.txt)
	else()
		add_subdirectory(libs)
	endif()
else()
	message(STATUS "======================================================")
	message(STATUS "Bundled libraries were not found on your system!")
	message(STATUS "======================================================")
	message(STATUS "You need the *multilib* package to crosscompile ET:L on a 64bit system.")
	message(STATUS "Alternatively clone etlegacy repository and then run")
	message(STATUS "        'git submodule init && git submodule update'")
	message(STATUS "and enable BUNDLED_* in CMake configuration.")
	message(FATAL_ERROR "Build stopped because of missing libraries.")
endif()
