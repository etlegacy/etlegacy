#-----------------------------------------------------------------
# Install GeoIP
#-----------------------------------------------------------------

message(STATUS "Installing GeoIP")

set(ETLEGACY_GEOIP_ARCHIVE "GeoIP.dat.gz")
set(ETLEGACY_GEOIP_ARCHIVE_URL "http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/GeoIP.dat.gz")

message(STATUS "Downloading GeoIP archive to ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}")
	file(DOWNLOAD
		${ETLEGACY_GEOIP_ARCHIVE_URL}
		"${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}"
		SHOW_PROGRESS TIMEOUT 30
	)
endif()

message(STATUS "Extracting GeoIP to ${CMAKE_CURRENT_BINARY_DIR}/legacy")
if(UNIX)
	execute_process(
		COMMAND gunzip ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
	)
elseif(WIN32 AND GZIP_EXECUTABLE)
	execute_process(
		COMMAND ${GZIP_EXECUTABLE} -d ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
	)
endif(UNIX)

message(STATUS "Adding GeoIP to installer scripts")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy/"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
)
