# Simple wrapper for locating cJSON package even if cmake config is not available
# that is missing on Fedora cjson-1.7.14-7.fc38 for example
find_package(cJSON CONFIG)

if (NOT cJSON_FOUND)
	find_package(PkgConfig)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(CJSON libcjson)
	endif()
else()
	set (CJSON_FOUND ${cJSON_FOUND})
endif()

if (NOT CJSON_FOUND)
	find_path(CJSON_INCLUDE_DIRS NAMES cjson.h PATH_SUFFIXES cjson)
	find_library(CJSON_LIBRARIES NAMES cjson)

	if (CJSON_INCLUDE_DIRS AND CJSON_LIBRARIES)
		message(STATUS "Found cJSON: ${CJSON_LIBRARIES}")
		set (CJSON_FOUND TRUE)
	endif()
	MARK_AS_ADVANCED(CJSON_INCLUDE_DIRS CJSON_LIBRARIES)
endif()
