#-----------------------------------------------------------------
# Build Server
#-----------------------------------------------------------------

add_executable(etlded ${COMMON_SRC} ${SERVER_SRC} ${PLATFORM_SRC} ${PLATFORM_SERVER_SRC})
target_link_libraries(etlded
	server_libraries
	engine_libraries
	os_libraries
)

set_target_properties(etlded
	PROPERTIES COMPILE_DEFINITIONS "DEDICATED"
	RUNTIME_OUTPUT_DIRECTORY ""
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ""
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ""
)

if((UNIX OR ETL_ARM) AND NOT APPLE AND NOT ANDROID)
	set_target_properties(etlded PROPERTIES SUFFIX "${BIN_SUFFIX}")
endif()

if(MSVC AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/etlded.vcxproj.user)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/etlded.vcxproj.user @ONLY)
endif()

install(TARGETS etlded RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}")
