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
	if(BUNDLED_SDL)
		# SDL's Android HID backend now builds C++ code, so the client shared
		# library has to link through the C++ driver to pull in the NDK runtime.
		set_target_properties(etl PROPERTIES LINKER_LANGUAGE CXX)
	endif()
	set(ETL_OUTPUT_DIR "legacy")
elseif(EMSCRIPTEN)
	add_executable(etl ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
	target_link_options(etl
			PRIVATE
				#"SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/misc/emscripten/shell_minimal.html"
				"SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/misc/emscripten/et_pak_loader.html"
				#"SHELL:--preload-file ${CMAKE_BINARY_DIR}/legacy/ui.mp.wasm32.so@/home/web_user/.etlegacy/legacy/ui.mp.wasm32.so"
				#"SHELL:--preload-file ${CMAKE_BINARY_DIR}/legacy/cgame.mp.wasm32.so@/home/web_user/.etlegacy/legacy/cgame.mp.wasm32.so"
				"SHELL:--preload-file ${CMAKE_BINARY_DIR}/legacy/qagame.mp.wasm32.so@/home/web_user/.etlegacy/legacy/qagame.mp.wasm32.so"
				#"SHELL:--preload-file ${CMAKE_BINARY_DIR}/ui.mp.wasm32.so@/ui.mp.wasm32.so"
				#"SHELL:--preload-file ${CMAKE_BINARY_DIR}/cgame.mp.wasm32.so@/cgame.mp.wasm32.so"
				#"SHELL:--preload-file ${CMAKE_BINARY_DIR}/etmain@etmain"
				"SHELL:--preload-file ${CMAKE_BINARY_DIR}/legacy@legacy"
				-sALLOW_MEMORY_GROWTH=1
				-sINITIAL_MEMORY=256MB
				-sMAXIMUM_MEMORY=2GB
				-sSTACK_SIZE=8MB
				-sALLOW_TABLE_GROWTH=1
				#--profiling
				-sFETCH
				-sERROR_ON_UNDEFINED_SYMBOLS=0
				-sMAIN_MODULE=1
				-sMINIFY_HTML=0
				-sEXPORTED_RUNTIME_METHODS=['callMain']
				-sEXIT_RUNTIME=1
				-sINITIAL_TABLE=500000
				-sFORCE_FILESYSTEM=1
				-lidbfs.js
			)
	set_target_properties(etl PROPERTIES SUFFIX ".html")
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

if((UNIX OR ETL_ARM) AND NOT APPLE AND NOT ANDROID AND NOT EMSCRIPTEN)
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
