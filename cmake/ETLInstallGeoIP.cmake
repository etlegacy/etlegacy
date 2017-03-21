#-----------------------------------------------------------------
# Install GeoIP
#-----------------------------------------------------------------

message(STATUS "Installing GeoIP")

if(UNIX OR (WIN32 AND UNZIP_EXECUTABLE))
	set(ETLEGACY_GEOIP_ARCHIVE "GeoIP.dat.gz")
	set(ETLEGACY_GEOIP_ARCHIVE_URL "http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/GeoIP.dat.gz")
endif()

set(ETLEGACY_GEOIP_DL_URL "${ETLEGACY_GEOIP_ARCHIVE_URL}")

message(STATUS "Downloading GeoIP archive to ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}")
	file(DOWNLOAD
		${ETLEGACY_GEOIP_DL_URL}
		"${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}"
		SHOW_PROGRESS TIMEOUT 30
	)
endif()

message(STATUS "Extracting GeoIP to ${CMAKE_CURRENT_BINARY_DIR}/legacy")
if(UNIX)
	execute_process(
		COMMAND tar -xf ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
	)
elseif(WIN32)
	execute_process(
		COMMAND ${UNZIP_EXECUTABLE} -u ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
	)

message(STATUS "Adding GeoIP to installer scripts")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
)
endif(UNIX)
