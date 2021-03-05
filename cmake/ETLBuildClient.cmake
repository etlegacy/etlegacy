#-----------------------------------------------------------------
# Build Client
#-----------------------------------------------------------------

if(WIN32)
	add_executable(etl WIN32 ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
elseif(APPLE)
	# These are vars used in the misc/Info.plist template file
	# See set_target_properties( ... MACOSX_BUNDLE_INFO_PLIST ...)
	set(MACOSX_BUNDLE_INFO_STRING            "ET: Legacy")
	set(MACOSX_BUNDLE_ICON_FILE              "etl.icns")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER         "com.etlegacy.etl")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING    "${ETL_CMAKE_VERSION}")
	set(MACOSX_BUNDLE_BUNDLE_NAME            "ETLegacy")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING   "${ETL_CMAKE_VERSION_SHORT}")
	set(MACOSX_BUNDLE_COPYRIGHT              "etlegacy.com")

	# Specify files to be copied into the .app's Resources folder
	set(RESOURCES_DIR "${CMAKE_SOURCE_DIR}/misc")
	set(MACOSX_RESOURCES "${RESOURCES_DIR}/${MACOSX_BUNDLE_ICON_FILE}")
	set_source_files_properties(${CMAKE_SOURCE_DIR}/misc/${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

	# Create the .app bundle
	add_executable(etl MACOSX_BUNDLE ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC} ${MACOSX_RESOURCES})
	set_target_properties(etl PROPERTIES OUTPUT_NAME "ET Legacy")
elseif(ANDROID)
	add_library(libetl SHARED ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
else()
	add_executable(etl ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
endif()

if(BUNDLED_SDL)
	add_dependencies(etl bundled_sdl)
endif()

if(BUNDLED_ZLIB)
	add_dependencies(etl bundled_zlib)
endif()

if(BUNDLED_MINIZIP)
	add_dependencies(etl bundled_minizip)
endif()

if(BUNDLED_OPENSSL)
	add_dependencies(etl bundled_openssl)
endif()

if(BUNDLED_WOLFSSL)
	add_dependencies(etl bundled_wolfssl)
endif()

if(BUNDLED_CURL)
	add_dependencies(etl bundled_curl)
endif()

if(BUNDLED_OGG_VORBIS)
	add_dependencies(etl bundled_ogg bundled_ogg_vorbis bundled_ogg_vorbis_file)
endif()

if(BUNDLED_THEORA)
	add_dependencies(bundled_theora bundled_ogg bundled_ogg_vorbis bundled_ogg_vorbis_file)
	add_dependencies(etl bundled_theora)
endif()

if(BUNDLED_OPENAL)
	add_dependencies(etl bundled_openal)
endif()

if(NOT ANDROID)
	target_link_libraries(etl
		${CLIENT_LIBRARIES}
		${SDL_LIBRARIES}
		${OS_LIBRARIES} # Has to go after cURL and SDL
	)
else()
	target_link_libraries(libetl
			${CLIENT_LIBRARIES}
			${SDL_LIBRARIES}
			ifaddrs
			ogg
			vorbis
			android
			)
endif()

if(FEATURE_WINDOWS_CONSOLE AND WIN32)
	set(ETL_COMPILE_DEF "USE_ICON;USE_WINDOWS_CONSOLE")
else()
	set(ETL_COMPILE_DEF "USE_ICON")
endif()

if(NOT ANDROID)
	set_target_properties(etl PROPERTIES
		COMPILE_DEFINITIONS "${ETL_COMPILE_DEF}"
		RUNTIME_OUTPUT_DIRECTORY ""
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ""
		RUNTIME_OUTPUT_DIRECTORY_RELEASE ""
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/Info.plist
	)
else()
	set_target_properties(libetl PROPERTIES
			COMPILE_DEFINITIONS "${ETL_COMPILE_DEF}"
			LIBRARY_OUTPUT_DIRECTORY "legacy"
			LIBRARY_OUTPUT_DIRECTORY_DEBUG "legacy"
			LIBRARY_OUTPUT_DIRECTORY_RELEASE "legacy"
			)
endif()

if(MSVC AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/etl.vcxproj.user)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/etl.vcxproj.user @ONLY)
endif()

if(NOT ANDROID)
install(TARGETS etl
	BUNDLE  DESTINATION "${INSTALL_DEFAULT_BINDIR}"
	RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}"
)
endif()
