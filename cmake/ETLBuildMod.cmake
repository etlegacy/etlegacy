#-----------------------------------------------------------------
# Build mod pack
#-----------------------------------------------------------------

# find libm where it exists and link game modules against it
include(CheckLibraryExists)
check_library_exists(m pow "" LIBM)

#
# cgame
#
add_library(cgame${LIB_SUFFIX}${ARCH} MODULE ${CGAME_SRC})
set_target_properties(cgame${LIB_SUFFIX}${ARCH}
	PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL"
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
)

if(LIBM)
	target_link_libraries(cgame${LIB_SUFFIX}${ARCH} PRIVATE m)
endif()

#
# qagame
#
add_library(qagame${LIB_SUFFIX}${ARCH} MODULE ${QAGAME_SRC})
if(FEATURE_LUASQL AND FEATURE_DBMS)
	target_compile_definitions(qagame${LIB_SUFFIX}${ARCH} PRIVATE FEATURE_DBMS FEATURE_LUASQL)

	if(BUNDLED_SQLITE3)
		add_dependencies(qagame${LIB_SUFFIX}${ARCH} bundled_sqlite3)
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
		add_dependencies(qagame${LIB_SUFFIX}${ARCH} bundled_lua)
	endif(BUNDLED_LUA)
	target_link_libraries(qagame${LIB_SUFFIX}${ARCH} ${MOD_LIBRARIES})
endif(FEATURE_LUA)



if(FEATURE_SERVERMDX)
	target_compile_definitions(qagame${LIB_SUFFIX}${ARCH} PRIVATE FEATURE_SERVERMDX)
endif()

target_compile_definitions(qagame${LIB_SUFFIX}${ARCH} PRIVATE GAMEDLL)

set_target_properties(qagame${LIB_SUFFIX}${ARCH}
	PROPERTIES
	# COMPILE_DEFINITIONS "${QAGAME_DEFINES}"
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
	RUNTIME_OUTPUT_DIRECTORY "${MODNAME}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
)

#
# ui
#
add_library(ui${LIB_SUFFIX}${ARCH} MODULE ${UI_SRC})
set_target_properties(ui${LIB_SUFFIX}${ARCH}
	PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
)

if(LIBM)
	target_link_libraries(ui${LIB_SUFFIX}${ARCH} PRIVATE m)
endif()

# Build both arhitectures on older xcode versions
if(APPLE)
	if(CMAKE_OSX_DEPLOYMENT_TARGET LESS_EQUAL "10.14")
		# Force universal mod on osx up to Mojave
		set(OSX_MOD_ARCH "i386;x86_64")
	else()
		# 64bit mod only as of Catalina and higher
		set(OSX_MOD_ARCH "x86_64")
	endif()

	set_target_properties(cgame${LIB_SUFFIX}${ARCH} PROPERTIES OSX_ARCHITECTURES "${OSX_MOD_ARCH}" )
	set_target_properties(ui${LIB_SUFFIX}${ARCH} PROPERTIES OSX_ARCHITECTURES "${OSX_MOD_ARCH}" )
endif()

# install bins of cgame, ui and qgame
if(BUILD_MOD_PK3)
	install(TARGETS qagame${LIB_SUFFIX}${ARCH}
		RUNTIME DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	)
else()
	install(TARGETS cgame${LIB_SUFFIX}${ARCH} qagame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
		RUNTIME DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	)
endif()

#
# mod pk3
#
if(BUILD_MOD_PK3)
	add_custom_target(mod_pk3 ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_${ETL_CMAKE_VERSION_SHORT}.pk3)

	# binaries
	if(APPLE)
		set(ZIP_FILE_LIST cgame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH})
	else()
		set(ZIP_FILE_LIST cgame${LIB_SUFFIX}${ARCH}${CMAKE_SHARED_LIBRARY_SUFFIX} ui${LIB_SUFFIX}${ARCH}${CMAKE_SHARED_LIBRARY_SUFFIX})
	endif()

	# etmain
	file(GLOB ETMAIN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/etmain/*")
	foreach(FILE ${ETMAIN_FILES})
		file(RELATIVE_PATH REL "${CMAKE_CURRENT_SOURCE_DIR}/etmain" ${FILE})
		list(APPEND ETMAIN_FILES_LIST ${REL})
	endforeach()

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_${ETL_CMAKE_VERSION_SHORT}.pk3
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/etmain ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}
		COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_${ETL_CMAKE_VERSION_SHORT}.pk3 --format=zip ${ZIP_FILE_LIST} ${ETMAIN_FILES_LIST}
		DEPENDS cgame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/
		VERBATIM
	)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_${ETL_CMAKE_VERSION_SHORT}.pk3
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	)
endif()
