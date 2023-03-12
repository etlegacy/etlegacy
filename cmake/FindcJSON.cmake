# Simple wrapper for locating cJSON package even if cmake config is not available
find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(CJSON libcjson)
	set (CJSON_LIBRARY ${CJSON_LIBRARIES})
endif()

if (NOT CJSON_FOUND)
	find_path(CJSON_INCLUDE_DIRS NAMES cjson.h PATH_SUFFIXES cjson)
	find_library(CJSON_LIBRARIES NAMES cjson)

	if (CJSON_INCLUDE_DIRS AND CJSON_LIBRARIES)
		message(STATUS "Found cJSON: ${CJSON_LIBRARIES}")
		set (CJSON_FOUND TRUE)
		# Should this one be name like all other variables too?
		set (CJSON_LIBRARY ${CJSON_LIBRARIES})
	endif()
endif()

MARK_AS_ADVANCED(CJSON_INCLUDE_DIRS CJSON_LIBRARIES CJSON_LIBRARY)
