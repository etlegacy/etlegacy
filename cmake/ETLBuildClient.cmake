#-----------------------------------------------------------------
# Build Client
#-----------------------------------------------------------------

if(WIN32)
	add_executable(etl WIN32 ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
elseif(APPLE)
	# These are vars used in the misc/Info.plist template file
	# See set_target_properties( ... MACOSX_BUNDLE_INFO_PLIST ...)
	set(MACOSX_BUNDLE_INFO_STRING            "Enemy Territory: Legacy")
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
else()
	add_executable(etl ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
endif(WIN32)

if(BUNDLED_SDL)
	add_dependencies(etl bundled_sdl)
endif(BUNDLED_SDL)

if(BUNDLED_ZLIB)
	add_dependencies(etl bundled_zlib)
endif(BUNDLED_ZLIB)

if(BUNDLED_MINIZIP)
	add_dependencies(etl bundled_minizip)
endif(BUNDLED_MINIZIP)

if(BUNDLED_CURL)
	add_dependencies(etl bundled_curl)
endif(BUNDLED_CURL)

if(BUNDLED_JANSSON)
	add_dependencies(etl bundled_jansson)
endif(BUNDLED_JANSSON)

if(BUNDLED_OGG_VORBIS)
	add_dependencies(etl bundled_ogg bundled_ogg_vorbis bundled_ogg_vorbis_file)
endif(BUNDLED_OGG_VORBIS)

if(BUNDLED_THEORA)
	add_dependencies(bundled_theora bundled_ogg bundled_ogg_vorbis bundled_ogg_vorbis_file)
	add_dependencies(etl bundled_theora)
endif(BUNDLED_THEORA)

if(BUNDLED_OPENAL)
	add_dependencies(etl bundled_openal)
endif(BUNDLED_OPENAL)

target_link_libraries(etl
	${CLIENT_LIBRARIES}
	${SDL_LIBRARIES}
	${OS_LIBRARIES} # Has to go after cURL and SDL
)

if(FEATURE_WINDOWS_CONSOLE AND WIN32)
	set(ETL_COMPILE_DEF "USE_ICON;USE_WINDOWS_CONSOLE")
else(FEATURE_WINDOWS_CONSOLE AND WIN32)
	set(ETL_COMPILE_DEF "USE_ICON")
endif(FEATURE_WINDOWS_CONSOLE AND WIN32)

set_target_properties(etl PROPERTIES
	COMPILE_DEFINITIONS "${ETL_COMPILE_DEF}"
	RUNTIME_OUTPUT_DIRECTORY ""
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ""
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ""
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/Info.plist
)

if(MSVC AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/etl.vcxproj.user)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/etl.vcxproj.user @ONLY)
endif()

install(TARGETS etl
	BUNDLE  DESTINATION "${INSTALL_DEFAULT_BINDIR}"
	RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}"
)
