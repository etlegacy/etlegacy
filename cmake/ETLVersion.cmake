# Version generation

# default values if they cannot be generated from git
set(ETLEGACY_VERSION_MAJOR "2")
set(ETLEGACY_VERSION_MINOR "71")
set(ETLEGACY_VERSION_PATCH "5")
set(ETLEGACY_VERSION "${ETLEGACY_VERSION_MAJOR}.${ETLEGACY_VERSION_MINOR}-dirty")
set(ETLEGACY_VERSIONPLAIN "${ETLEGACY_VERSION_MAJOR},${ETLEGACY_VERSION_MINOR},${ETLEGACY_VERSION_PATCH},0")

macro(HEXCHAR2DEC VAR VAL)
	if (${VAL} MATCHES "[0-9]")
	  SET(${VAR} ${VAL})
	elseif(${VAL} MATCHES "[aA]")
	  SET(${VAR} 10)
	elseif(${VAL} MATCHES "[bB]")
	  SET(${VAR} 11)
	elseif(${VAL} MATCHES "[cC]")
	  SET(${VAR} 12)
	elseif(${VAL} MATCHES "[dD]")
	  SET(${VAR} 13)
	elseif(${VAL} MATCHES "[eE]")
	  SET(${VAR} 14)
	elseif(${VAL} MATCHES "[fF]")
	  SET(${VAR} 15)
	else()
	  MESSAGE(FATAL_ERROR "Invalid format for hexidecimal character")
	endif()
endmacro(HEXCHAR2DEC)

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
		elseif("${GIT_DESCRIBE}" MATCHES "^v[0-9]+\\.[0-9]+[a-zA-Z]+.*")
			string(REGEX REPLACE "^v[0-9]+\\.[0-9]+([a-zA-Z]+).*" "\\1" VERSION_PATCH "${GIT_DESCRIBE}")
			HEXCHAR2DEC(VERSION_PATCH ${VERSION_PATCH})
		elseif("${GIT_DESCRIBE}" MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9a-zA-Z]+.*")
			string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9a-zA-Z]+).*" "\\1" VERSION_PATCH "${GIT_DESCRIBE}")
			HEXCHAR2DEC(VERSION_PATCH ${VERSION_PATCH})
		else()
			set(VERSION_PATCH 0)
		endif()

		set(ETL_CMAKE_PROD_VERSION "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},0")
		set(ETL_CMAKE_PROD_VERSIONSTR "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

		set(ETLEGACY_VERSION_MAJOR "${VERSION_MAJOR}")
		set(ETLEGACY_VERSION_MINOR "${VERSION_MINOR}")
		set(ETLEGACY_VERSION_PATCH "${VERSION_PATCH}")
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

#message(FATAL_ERROR "Tulee: ${ETL_CMAKE_VERSION}, ${ETL_CMAKE_VERSION_SHORT}, ${ETL_CMAKE_PROD_VERSION}, ${ETL_CMAKE_PROD_VERSIONSTR}")

# Mod version
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/git_version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/etmain/ui/git_version.h" @ONLY)
# This is for NSIS
string(REPLACE "," "." ETL_CMAKE_PROD_VERSIONDOT ${ETL_CMAKE_PROD_VERSION})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/git_version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/git_version.h" @ONLY)
list(APPEND COMMON_SRC "${CMAKE_CURRENT_BINARY_DIR}/include/git_version.h")
include_directories(${PROJECT_BINARY_DIR}/include) # git_version.h
