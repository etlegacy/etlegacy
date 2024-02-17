#-----------------------------------------------------------------
# Build Client
#-----------------------------------------------------------------

set(ETL_OUTPUT_DIR "")

if(WIN32)
	add_executable(etl WIN32 ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
elseif(APPLE)
	# These are vars used in the misc/Info.plist template file
	# See set_target_properties( ... MACOSX_BUNDLE_INFO_PLIST ...)
	set(MACOSX_BUNDLE_INFO_STRING            "ET: Legacy")
	set(MACOSX_BUNDLE_ICON_FILE              "etl.icns")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER         "com.etlegacy.etl")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING    "${ETL_CMAKE_VERSION}")
	set(MACOSX_BUNDLE_BUNDLE_NAME            "ET Legacy")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING   "${ETL_CMAKE_VERSION_SHORT}")
	set(MACOSX_BUNDLE_COPYRIGHT              "etlegacy.com")

	# Specify files to be copied into the .app's Resources folder
	set(RESOURCES_DIR "${CMAKE_SOURCE_DIR}/misc")
	set(MACOSX_RESOURCES "${RESOURCES_DIR}/${MACOSX_BUNDLE_ICON_FILE}")
	set_source_files_properties(${CMAKE_SOURCE_DIR}/misc/${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

	# Create the .app bundle
	add_executable(etl MACOSX_BUNDLE ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC} ${MACOSX_RESOURCES})
	set_target_properties(etl PROPERTIES
			OUTPUT_NAME "ET Legacy"
			XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME TRUE
			XCODE_ATTRIBUTE_EXECUTABLE_NAME "etl"
			MACOSX_BUNDLE_EXECUTABLE_NAME "etl"
	)
elseif(ANDROID)
	add_library(etl SHARED ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
	set_target_properties(etl PROPERTIES PREFIX "lib")
	set(ETL_OUTPUT_DIR "legacy")
else()
	add_executable(etl ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
endif()

target_link_libraries(etl
	client_libraries
	engine_libraries
	os_libraries # Has to go after cURL and SDL
)

if(FEATURE_WINDOWS_CONSOLE AND WIN32)
	set(ETL_COMPILE_DEF "USE_ICON;USE_WINDOWS_CONSOLE")
else()
	set(ETL_COMPILE_DEF "USE_ICON")
endif()

set_target_properties(etl PROPERTIES
	COMPILE_DEFINITIONS "${ETL_COMPILE_DEF}"
	RUNTIME_OUTPUT_DIRECTORY "${ETL_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${ETL_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ETL_OUTPUT_DIR}"
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/Info.plist
)

if((UNIX OR ETL_ARM) AND NOT APPLE AND NOT ANDROID)
	set_target_properties(etl PROPERTIES SUFFIX "${BIN_SUFFIX}")
endif()

target_compile_definitions(etl PRIVATE ETL_CLIENT=1)

if(MSVC AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/etl.vcxproj.user)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/etl.vcxproj.user @ONLY)
endif()

install(TARGETS etl
	BUNDLE  DESTINATION "${INSTALL_DEFAULT_BINDIR}"
	RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}"
)
