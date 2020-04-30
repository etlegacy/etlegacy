#-----------------------------------------------------------------
# Build mod pack
#-----------------------------------------------------------------

#
# cgame
#
add_library(cgame${LIB_SUFFIX}${ARCH} MODULE ${CGAME_SRC})
set_target_properties(cgame${LIB_SUFFIX}${ARCH}
	PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL"
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "legacy"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "legacy"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "legacy"
)

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
	LIBRARY_OUTPUT_DIRECTORY "legacy"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "legacy"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "legacy"
	RUNTIME_OUTPUT_DIRECTORY "legacy"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "legacy"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "legacy"
)

#
# ui
#
add_library(ui${LIB_SUFFIX}${ARCH} MODULE ${UI_SRC})
set_target_properties(ui${LIB_SUFFIX}${ARCH}
	PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "legacy"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "legacy"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "legacy"
)

# Build both arhitectures on older xcode versions
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET LESS_EQUAL "10.12" AND NOT OSX_NOX86)
	set_target_properties(cgame${LIB_SUFFIX}${ARCH}
		PROPERTIES
		OSX_ARCHITECTURES "i386;x86_64"
	)
	set_target_properties(ui${LIB_SUFFIX}${ARCH}
		PROPERTIES
		OSX_ARCHITECTURES "i386;x86_64"
	)
endif()

# install bins of cgame, ui and qgame
if(BUILD_MOD_PK3)
	install(TARGETS qagame${LIB_SUFFIX}${ARCH}
		RUNTIME DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
		LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
		ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
	)
else()
	install(TARGETS cgame${LIB_SUFFIX}${ARCH} qagame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
		RUNTIME DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
		LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
		ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
	)
endif()

#
# mod pk3
#
if(BUILD_MOD_PK3)
	add_custom_target(mod_pk3 ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/legacy/legacy_${ETL_CMAKE_VERSION_SHORT}.pk3)

	# binaries
	if(APPLE)
		set(ZIP_FILE_LIST cgame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH})
	else()
		set(ZIP_FILE_LIST cgame${LIB_SUFFIX}${ARCH}${CMAKE_SHARED_LIBRARY_SUFFIX} ui${LIB_SUFFIX}${ARCH}${CMAKE_SHARED_LIBRARY_SUFFIX})
	endif()

	# etmain
	file(GLOB ETMAIN_FILES_IN "${CMAKE_CURRENT_SOURCE_DIR}/etmain/*")
	foreach(FILE ${ETMAIN_FILES_IN})
		file(RELATIVE_PATH REL "${CMAKE_CURRENT_SOURCE_DIR}/etmain" ${FILE})
		list(APPEND ETMAIN_FILES_LIST ${REL})
		file(COPY ${FILE} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/legacy")
	endforeach()

	list(APPEND ZIP_FILE_LIST ${ETMAIN_FILES_LIST})

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/legacy/legacy_${ETL_CMAKE_VERSION_SHORT}.pk3
		COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_CURRENT_BINARY_DIR}/legacy/legacy_${ETL_CMAKE_VERSION_SHORT}.pk3 --format=zip ${ZIP_FILE_LIST}
		DEPENDS cgame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy/
		VERBATIM
	)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/legacy/legacy_${ETL_CMAKE_VERSION_SHORT}.pk3
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
	)
endif()
