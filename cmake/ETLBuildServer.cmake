#-----------------------------------------------------------------
# Build Server
#-----------------------------------------------------------------

add_executable(etlded ${COMMON_SRC} ${SERVER_SRC} ${PLATFORM_SRC} ${PLATFORM_SERVER_SRC})
target_link_libraries(etlded
	${SERVER_LIBRARIES}
	${OS_LIBRARIES}
)

set_target_properties(etlded
	PROPERTIES COMPILE_DEFINITIONS "DEDICATED"
	RUNTIME_OUTPUT_DIRECTORY ""
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ""
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ""
)

if(BUNDLED_ZLIB)
	add_dependencies(etlded bundled_zlib)
endif()

if(BUNDLED_MINIZIP)
	add_dependencies(etlded bundled_minizip)
endif()

if(BUNDLED_OPENSSL)
	add_dependencies(etlded bundled_openssl)
endif()

if(BUNDLED_WOLFSSL)
	add_dependencies(etlded bundled_wolfssl)
endif()

if(BUNDLED_CURL)
	add_dependencies(etlded bundled_curl)
endif()

if(FEATURE_DBMS)
	if(BUNDLED_SQLITE3)
		add_dependencies(etlded bundled_sqlite3)
	endif(BUNDLED_SQLITE3)
endif()

if(MSVC AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/etlded.vcxproj.user)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/etlded.vcxproj.user @ONLY)
endif()

install(TARGETS etlded RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}")
