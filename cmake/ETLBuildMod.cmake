#-----------------------------------------------------------------
# Build mod pack
#-----------------------------------------------------------------

# find libm where it exists and link game modules against it
include(CheckLibraryExists)
check_library_exists(m pow "" LIBM)
if(LIBM)
    target_link_libraries(cgame_libraries INTERFACE m)
    target_link_libraries(ui_libraries INTERFACE m)
endif()

#
# cgame
#
add_library(cgame MODULE ${CGAME_SRC})
target_link_libraries(cgame cgame_libraries mod_libraries)

set_target_properties(cgame
	PROPERTIES
	PREFIX ""
	C_STANDARD 90
	OUTPUT_NAME "cgame${LIB_SUFFIX}${ARCH}"
	LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
)
target_compile_definitions(cgame PRIVATE CGAMEDLL=1 MODLIB=1)

#
# qagame
#
if(NOT ANDROID)
	add_library(qagame MODULE ${QAGAME_SRC})
	target_link_libraries(qagame qagame_libraries mod_libraries)

	if(FEATURE_LUASQL AND FEATURE_DBMS)
		target_compile_definitions(qagame PRIVATE FEATURE_DBMS FEATURE_LUASQL)

		if(BUNDLED_SQLITE3)
			target_link_libraries(qagame bundled_sqlite3)
		else() # BUNDLED_SQLITE3
			find_package(SQLite3 REQUIRED)
			target_link_libraries(qagame ${SQLITE3_LIBRARY})
			target_include_directories(qagame PUBLIC ${SQLITE3_INCLUDE_DIR})
		endif()

		FILE(GLOB LUASQL_SRC
			"src/luasql/luasql.c"
			"src/luasql/luasql.h"
			"src/luasql/ls_sqlite3.c"
		)
		set(QAGAME_SRC ${QAGAME_SRC} ${LUASQL_SRC})
	endif()

	if(FEATURE_SERVERMDX)
		target_compile_definitions(qagame PRIVATE FEATURE_SERVERMDX)
	endif()

	set_target_properties(qagame
		PROPERTIES
		PREFIX ""
		C_STANDARD 90
		OUTPUT_NAME "qagame${LIB_SUFFIX}${ARCH}"
		LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
		LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
		LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
	)
	target_compile_definitions(qagame PRIVATE GAMEDLL=1 MODLIB=1)
endif()

#
# ui
#
add_library(ui MODULE ${UI_SRC})
target_link_libraries(ui ui_libraries mod_libraries)

set_target_properties(ui
	PROPERTIES
	PREFIX ""
	C_STANDARD 90
	OUTPUT_NAME "ui${LIB_SUFFIX}${ARCH}"
	LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
)
target_compile_definitions(ui PRIVATE UIDLL=1 MODLIB=1)

# Build both architectures on older xcode versions
if(APPLE)

	if (DEFINED CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "")
		message(STATUS "Using the user provided osx architectures: ${CMAKE_OSX_ARCHITECTURES}")
		set(OSX_MOD_ARCH "${CMAKE_OSX_ARCHITECTURES}")
	else()

		execute_process(
			COMMAND uname -m
			OUTPUT_VARIABLE ETL_OSX_NATIVE_ARCHITECTURE
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		check_c_compiler_flag("-arch i386" i386Supported)
		check_c_compiler_flag("-arch x86_64" x86_64Supported)
		check_c_compiler_flag("-arch arm64" arm64Supported)

		# Mojave was the last version to support 32 bit binaries and building.
		# Newer SDK's just fail compilation
		# TODO: maybe remove this whole thing after the next release.
		if(XCODE_SDK_VERSION LESS "10.14" AND CMAKE_OSX_DEPLOYMENT_TARGET LESS "10.14" AND i386Supported AND x86_64Supported)
			# Force universal mod on osx up to Mojave
			message(STATUS "Enabling MacOS x86 and x86_64 builds on mods")
			set(OSX_MOD_ARCH "i386;x86_64")
		elseif(XCODE_SDK_VERSION GREATER_EQUAL "11.00" AND x86_64Supported AND arm64Supported)
			message(STATUS "Enabling MacOS x86_64 and Arm builds on mods")
			set(OSX_MOD_ARCH "x86_64;arm64")
		else()
			# Using only the native arch
			message(STATUS "Only doing MacOS ${ETL_OSX_NATIVE_ARCHITECTURE} bit build")
			set(OSX_MOD_ARCH "${ETL_OSX_NATIVE_ARCHITECTURE}")
		endif()

	endif()

	set_target_properties(cgame PROPERTIES OSX_ARCHITECTURES "${OSX_MOD_ARCH}" )
	set_target_properties(ui PROPERTIES OSX_ARCHITECTURES "${OSX_MOD_ARCH}" )
elseif(ANDROID)
	set_target_properties(cgame PROPERTIES PREFIX "lib")
	set_target_properties(ui PROPERTIES PREFIX "lib")
endif()

# install bins of cgame, ui and qgame
if(NOT ANDROID)
	if(BUILD_MOD_PK3)
		install(TARGETS qagame
			RUNTIME DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
			LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		)
	else()
		install(TARGETS cgame qagame ui
			RUNTIME DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
			LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		)
	endif()
endif()

#
# mod pk3
#
if(BUILD_MOD_PK3)
	add_custom_target(mod_pk3 ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_${ETL_CMAKE_VERSION_SHORT}.pk3)

	# etmain
	file(GLOB ETMAIN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/etmain/*")
	foreach(FILE ${ETMAIN_FILES})
		file(RELATIVE_PATH REL "${CMAKE_CURRENT_SOURCE_DIR}/etmain" ${FILE})
		list(APPEND ETMAIN_FILES_LIST ${REL})
	endforeach()

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_${ETL_CMAKE_VERSION_SHORT}.pk3
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/etmain ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}
		COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_${ETL_CMAKE_VERSION_SHORT}.pk3 --format=zip $<TARGET_FILE_NAME:ui> $<TARGET_FILE_NAME:cgame> ${ETMAIN_FILES_LIST}
		DEPENDS cgame ui
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/
		VERBATIM
	)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_${ETL_CMAKE_VERSION_SHORT}.pk3
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	)
endif()
