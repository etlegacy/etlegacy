#-----------------------------------------------------------------
# Install GeoIP
#-----------------------------------------------------------------

message(STATUS "Installing GeoIP")

# set(ETLEGACY_GEOIP_ARCHIVE "GeoIP.dat.gz")
# set(ETLEGACY_GEOIP_ARCHIVE_URL "http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/GeoIP.dat.gz")
set(ETLEGACY_GEOIP_ARCHIVE "GeoIP.dat.zip")
set(ETLEGACY_GEOIP_ARCHIVE_URL "https://mirror.etlegacy.com/GeoIP.dat.zip")

message(STATUS "Downloading GeoIP archive to ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}")
	file(DOWNLOAD
		${ETLEGACY_GEOIP_ARCHIVE_URL}
		"${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}"
		SHOW_PROGRESS TIMEOUT 30
	)
endif()

message(STATUS "Extracting GeoIP to ${CMAKE_CURRENT_BINARY_DIR}/legacy")
execute_process(
	COMMAND ${CMAKE_COMMAND} -E tar -xzf ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_GEOIP_ARCHIVE}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
)

message(STATUS "Adding GeoIP to installer scripts")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/legacy/GeoIP.dat"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
)
