#-----------------------------------------------------------------
# Install WolfAdmin
#-----------------------------------------------------------------

message(STATUS "Installing WolfAdmin")

set(ETLEGACY_WOLFADMIN_ARCHIVE "wolfadmin.tar.gz")
set(ETLEGACY_WOLFADMIN_ARCHIVE_URL "http://mirror.etlegacy.com/wolfadmin/wolfadmin.tar.gz")

message(STATUS "Downloading WolfAdmin archive to ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_WOLFADMIN_ARCHIVE}")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_WOLFADMIN_ARCHIVE}")
	file(DOWNLOAD
		${ETLEGACY_WOLFADMIN_ARCHIVE_URL}
		"${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_WOLFADMIN_ARCHIVE}"
		SHOW_PROGRESS TIMEOUT 30
	)
endif()

message(STATUS "Extracting WolfAdmin to ${CMAKE_CURRENT_BINARY_DIR}/legacy")
if(UNIX)
	execute_process(
		COMMAND tar -xf ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_WOLFADMIN_ARCHIVE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
	)
elseif(WIN32 AND GZIP_EXECUTABLE)
	execute_process(
		COMMAND ${GZIP_EXECUTABLE} -d ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_WOLFADMIN_ARCHIVE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
	)
endif(UNIX)

file(COPY "${CMAKE_CURRENT_BINARY_DIR}/legacy/wolfadmin/luamods"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/legacy"
)
file(COPY "${CMAKE_CURRENT_BINARY_DIR}/legacy/wolfadmin/lualibs"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/legacy"
)
file(GLOB ETLEGACY_WOLFADMIN_CONFIG
	"${CMAKE_CURRENT_BINARY_DIR}/legacy/wolfadmin/config/*.toml"
)
foreach(ETLEGACY_WOLFADMIN_CONFIG ${ETLEGACY_WOLFADMIN_CONFIG})
	file(COPY "${ETLEGACY_WOLFADMIN_CONFIG}"
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/legacy"
	)
endforeach()

message(STATUS "Adding WolfAdmin to installer scripts")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy/luamods"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy/lualibs"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
)
foreach(ETLEGACY_WOLFADMIN_CONFIG ${ETLEGACY_WOLFADMIN_CONFIG})
	install(FILES "${ETLEGACY_WOLFADMIN_CONFIG}"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
	)
endforeach()
