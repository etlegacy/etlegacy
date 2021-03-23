#-----------------------------------------------------------------
# Build mod pack
#-----------------------------------------------------------------

# find libm where it exists and link game modules against it
include(CheckLibraryExists)
check_library_exists(m pow "" LIBM)

#
# cgame
#
add_library(cgame MODULE ${CGAME_SRC})
set_target_properties(cgame
	PROPERTIES
	PREFIX ""
	OUTPUT_NAME "cgame${LIB_SUFFIX}${ARCH}"
	LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
)
target_compile_definitions(cgame PRIVATE CGAMEDLL=1 MODLIB=1)

if(LIBM)
	target_link_libraries(cgame PRIVATE m)
endif()

#
# qagame
#
if(NOT ANDROID)
	add_library(qagame MODULE ${QAGAME_SRC})
	if(FEATURE_LUASQL AND FEATURE_DBMS)
		target_compile_definitions(qagame PRIVATE FEATURE_DBMS FEATURE_LUASQL)

		if(BUNDLED_SQLITE3)
			add_dependencies(qagame bundled_sqlite3)
			list(APPEND MOD_LIBRARIES ${SQLITE3_BUNDLED_LIBRARIES})
			include_directories(SYSTEM ${SQLITE3_BUNDLED_INCLUDE_DIR})
		else() # BUNDLED_SQLITE3
			find_package(SQLite3 REQUIRED)
			list(APPEND MOD_LIBRARIES ${SQLITE3_LIBRARY})
			include_directories(SYSTEM ${SQLITE3_INCLUDE_DIR})
		endif()

		FILE(GLOB LUASQL_SRC
			"src/luasql/luasql.c"
			"src/luasql/luasql.h"
			"src/luasql/ls_sqlite3.c"
		)
		set(QAGAME_SRC ${QAGAME_SRC} ${LUASQL_SRC})
	endif()

	if(FEATURE_LUA)
		if(BUNDLED_LUA)
			add_dependencies(qagame bundled_lua)
		endif(BUNDLED_LUA)
		target_link_libraries(qagame ${MOD_LIBRARIES})
	endif(FEATURE_LUA)



	if(FEATURE_SERVERMDX)
		target_compile_definitions(qagame PRIVATE FEATURE_SERVERMDX)
	endif()


	set_target_properties(qagame
		PROPERTIES
		# COMPILE_DEFINITIONS "${QAGAME_DEFINES}"
		PREFIX ""
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
set_target_properties(ui
	PROPERTIES
	PREFIX ""
	OUTPUT_NAME "ui${LIB_SUFFIX}${ARCH}"
	LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
)
target_compile_definitions(ui PRIVATE UIDLL=1 MODLIB=1)

if(LIBM)
	target_link_libraries(ui PRIVATE m)
endif()

# Build both arhitectures on older xcode versions
if(APPLE)
	# Mojave was the last version to support 32 bit binaries and building.
	# Newer SDK's just fail compilation
	# TODO: maybe remove this whole thing after the next release.
	if(XCODE_SDK_VERSION LESS "10.14" AND CMAKE_OSX_DEPLOYMENT_TARGET LESS "10.14")
		# Force universal mod on osx up to Mojave
		message(STATUS "Enabling MacOS x86 and x86_64 builds on mods")
		set(OSX_MOD_ARCH "i386;x86_64")
	elseif(XCODE_SDK_VERSION GREATER_EQUAL "11.00")
		message(STATUS "Enabling MacOS x86_64 and Arm builds on mods")
		set(OSX_MOD_ARCH "x86_64;arm64")
	else()
		# 64bit mod only as of Catalina and higher
		message(STATUS "Only doing MacOS x86_64 bit build")
		set(OSX_MOD_ARCH "x86_64")
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
