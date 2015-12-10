add_executable(etlded ${COMMON_SRC} ${MINIZIP_SRC} ${ZLIB_SRC} ${SERVER_SRC} ${PLATFORM_SRC} ${PLATFORM_SERVER_SRC})
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
endif(BUNDLED_ZLIB)

if(BUNDLED_MINIZIP)
	add_dependencies(etlded bundled_minizip)
endif(BUNDLED_MINIZIP)

if(BUNDLED_CURL)
	add_dependencies(etlded bundled_curl)
endif(BUNDLED_CURL)

if(FEATURE_DBMS)
	if(BUNDLED_SQLITE3)
		add_dependencies(etlded bundled_sqlite3)
	endif(BUNDLED_SQLITE3)
endif()

install(TARGETS etlded RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}")
