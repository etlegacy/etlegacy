# Version generation
git_describe(GIT_DESCRIBE)
git_describe(GIT_DESCRIBE_TAG "--abbrev=0")
if(GIT_DESCRIBE)
	set(ETL_CMAKE_VERSION ${GIT_DESCRIBE})
	set(ETL_CMAKE_VERSION_SHORT ${GIT_DESCRIBE_TAG})

	if("${GIT_DESCRIBE}" MATCHES "^v[0-9]+\\.[0-9]+.*")
		string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${GIT_DESCRIBE}")
		string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${GIT_DESCRIBE}")

		if("${GIT_DESCRIBE}" MATCHES "^v[0-9]+\\.[0-9]+rc[0-9]+.*")
			string(REGEX REPLACE "^v[0-9]+\\.[0-9]+rc([0-9]+).*" "\\1" VERSION_PATCH "${GIT_DESCRIBE}")
		else()
			set(VERSION_PATCH 0)
		endif()

		set(ETL_CMAKE_PROD_VERSION "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},0")
		set(ETL_CMAKE_PROD_VERSIONSTR "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
	else()
		set(ETL_CMAKE_PROD_VERSION ${ETLEGACY_VERSIONPLAIN})
		set(ETL_CMAKE_PROD_VERSIONSTR ${ETLEGACY_VERSION})
	endif()
else() # Not using source from git repo
	set(ETL_CMAKE_VERSION ${ETLEGACY_VERSION})
	set(ETL_CMAKE_VERSION_SHORT ${ETLEGACY_VERSION})
	set(ETL_CMAKE_PROD_VERSION ${ETLEGACY_VERSIONPLAIN})
	set(ETL_CMAKE_PROD_VERSIONSTR ${ETLEGACY_VERSION})
endif()
# Mod version
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/git_version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/etmain/ui/git_version.h" @ONLY)
# This is for NSIS
string(REPLACE "," "." ETL_CMAKE_PROD_VERSIONDOT ${ETL_CMAKE_PROD_VERSION})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/git_version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/git_version.h" @ONLY)
list(APPEND COMMON_SRC "${CMAKE_CURRENT_BINARY_DIR}/include/git_version.h")
include_directories(${PROJECT_BINARY_DIR}/include) # git_version.h
