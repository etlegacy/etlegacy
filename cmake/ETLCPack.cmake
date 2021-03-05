#-----------------------------------------------------------------
# CPack
#-----------------------------------------------------------------

# TODO: move this to include(EtlegacyCPack)
# CPack general configuration
set(CPACK_PACKAGE_NAME                "etlegacy")
set(CPACK_BUNDLE_NAME                 "etlegacy")
set(CPACK_PACKAGE_FILE_NAME           "etlegacy")
set(CPACK_BUNDLE_STARTUP_COMMAND      "etl")
set(CPACK_PACKAGE_ICON                "${CMAKE_SOURCE_DIR}/misc/etl.icns")
set(CPACK_BUNDLE_ICON                 "${CMAKE_SOURCE_DIR}/misc/etl.icns")
set(CPACK_BUNDLE_PLIST                "${CMAKE_SOURCE_DIR}/misc/Info.plist")
set(CPACK_PACKAGE_VENDOR              "ET: Legacy")
set(CPACK_PACKAGE_CONTACT             "mail@etlegacy.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ET: Legacy is an online FPS game compatible with Wolfenstein: Enemy Territory 2.60b.")
set(CPACK_PACKAGE_DESCRIPTION         ${CPACK_PACKAGE_DESCRIPTION_SUMMARY}) # TODO: expand
set(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_SOURCE_DIR}/docs/INSTALL.txt")
#set(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_SOURCE_DIR}/COPYING.txt") # FIXME: breaks bundle generator
set(CPACK_PACKAGE_VERSION_MAJOR       ${ETLEGACY_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR       ${ETLEGACY_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH       ${ETLEGACY_VERSION_PATCH})

if(WIN32)
	set(CPACK_GENERATOR "NSIS" "ZIP")
	set(CPACK_PACKAGE_VERSION ${ETL_CMAKE_PROD_VERSION_STR})
	string(TIMESTAMP CPACK_PACKAGE_YEAR "%Y" UTC)
	set(CPACK_SOURCE_IGNORE_FILES "etl.ico")
	if(FEATURE_RENDERER2)
		set(CPACK_RENDERER2_ACTIVE "1")
	else()
		set(CPACK_RENDERER2_ACTIVE "0")
	endif()
endif()

if(UNIX)
	if(APPLE)
		set(CPACK_GENERATOR "TGZ")
	else()
		set(CPACK_GENERATOR "TGZ" "STGZ")
	endif()
	set(CPACK_PACKAGE_VERSION ${ETL_CMAKE_VERSION})
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING.txt") # FIXME: move above
endif()

if(APPLE)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${ETL_CMAKE_VERSION}-macOS")
else()
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${ETL_CMAKE_VERSION}-${ARCH}")
endif()
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake")

# Probably only in use with the CI
if(ZIP_ONLY)
	set(CPACK_GENERATOR "ZIP")
endif()

# CPack generator-specific configuration
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackOptions.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake"
	IMMEDIATE @ONLY
)

include(CPack) # Has to be included after the package configuration!
