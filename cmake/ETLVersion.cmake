#-----------------------------------------------------------------
# Version
#-----------------------------------------------------------------

# default values if they cannot be generated from git
set(ETLEGACY_VERSION_MAJOR "2")
set(ETLEGACY_VERSION_MINOR "75")
set(ETLEGACY_VERSION_PATCH "0")
set(ETLEGACY_VERSION_COMMIT "0")
set(ETLEGACY_VERSION "${ETLEGACY_VERSION_MAJOR}.${ETLEGACY_VERSION_MINOR}-dirty")
set(ETLEGACY_VERSIONPLAIN "${ETLEGACY_VERSION_MAJOR},${ETLEGACY_VERSION_MINOR},${ETLEGACY_VERSION_PATCH},${ETLEGACY_VERSION_COMMIT}")

macro(HEXCHAR2DEC VAR VAL)
	if(${VAL} MATCHES "[0-9]")
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

macro(GENERATENUMBER VAR VAL)
	IF(${VAL} EQUAL 0)
		SET(${VAR} 0)
	ELSEIF(${VAL} MATCHES "^[0-9]+$") # if its just numbers we escape out and just use that
		SET(${VAR} ${VAL})
	ELSE()
		SET(CURINDEX 0)
		STRING(LENGTH "${VAL}" CURLENGTH)
		SET(${VAR} 0)
		WHILE(CURINDEX LESS  CURLENGTH)
			STRING(SUBSTRING "${VAL}" ${CURINDEX} 1 CHAR)
			HEXCHAR2DEC(CHAR ${CHAR})
			MATH(EXPR POWAH "(1<<((${CURLENGTH}-${CURINDEX}-1)*4))")
			MATH(EXPR CHAR "(${CHAR}*${POWAH})")
			MATH(EXPR ${VAR} "${${VAR}}+${CHAR}")
			MATH(EXPR CURINDEX "${CURINDEX}+1")
		ENDWHILE()
	ENDIF()
endmacro(GENERATENUMBER)

git_describe(GIT_DESCRIBE)
git_describe(GIT_DESCRIBE_TAG "--abbrev=0")
if(GIT_DESCRIBE)
	set(ETL_CMAKE_VERSION ${GIT_DESCRIBE})
	set(ETL_CMAKE_VERSION_SHORT ${GIT_DESCRIBE_TAG})

	string(COMPARE EQUAL "${ETL_CMAKE_VERSION}" "${ETL_CMAKE_VERSION_SHORT}" VERSION_NOT_DIRTY)

	if(NOT VERSION_NOT_DIRTY)
		set(ETL_CMAKE_VERSION_SHORT "${GIT_DESCRIBE_TAG}_dirty")
	endif()

	if("${GIT_DESCRIBE}" MATCHES "^v[0-9]+\\.[0-9]+.*")
		string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${GIT_DESCRIBE}")
		string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${GIT_DESCRIBE}")

		if("${GIT_DESCRIBE}" MATCHES "^v[0-9]+\\.[0-9]+rc[0-9]+.*")
			string(REGEX REPLACE "^v[0-9]+\\.[0-9]+rc([0-9]+).*" "\\1" VERSION_PATCH "${GIT_DESCRIBE}")
		elseif("${GIT_DESCRIBE}" MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9a-zA-Z]+.*")
			string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.?([0-9a-zA-Z]+).*" "\\1" VERSION_PATCH "${GIT_DESCRIBE}")
			GENERATENUMBER(VERSION_PATCH ${VERSION_PATCH})
		else()
			set(VERSION_PATCH 0)
		endif()


		if("${GIT_DESCRIBE}" MATCHES "^v[0-9]+\\.[0-9]+.*\\-[0-9]+\\-[0-9a-zA-Z]+")
			string(REGEX REPLACE "^v[0-9]+\\.[0-9]+.*\\-([0-9]+)\\-[0-9a-zA-Z]+" "\\1" VERSION_COMMIT_OFFSET "${GIT_DESCRIBE}")
		else()
			set(VERSION_COMMIT_OFFSET 0)
		endif()

		set(ETL_CMAKE_PROD_VERSION "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},${VERSION_COMMIT_OFFSET}")
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

if(${ETLEGACY_VERSION_MINOR} LESS 10)
	set(ETL_CMAKE_VERSION_INT "${ETLEGACY_VERSION_MAJOR}0${ETLEGACY_VERSION_MINOR}")
else()
	set(ETL_CMAKE_VERSION_INT "${ETLEGACY_VERSION_MAJOR}${ETLEGACY_VERSION_MINOR}")
endif()

if(NOT CMAKE_VERSION VERSION_LESS 3.0.2)
	string(TIMESTAMP ETL_CMAKE_BUILD_TIME "%Y-%m-%dT%H:%M:%S" UTC)
else()
	set(ETL_CMAKE_BUILD_TIME "1999-01-01T00:00:00") # Yes this is a joke, for the systems running ancient cmake versions
endif()

# Mod version
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/git_version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/etmain/ui/git_version.h" @ONLY)
# This is for NSIS
string(REPLACE "," "." ETL_CMAKE_PROD_VERSIONDOT ${ETL_CMAKE_PROD_VERSION})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/git_version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/git_version.h" @ONLY)
list(APPEND COMMON_SRC "${CMAKE_CURRENT_BINARY_DIR}/include/git_version.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include) # git_version.h
