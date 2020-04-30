#-----------------------------------------------------------------
# Install WolfAdmin
#-----------------------------------------------------------------

message(STATUS "Installing WolfAdmin")

set(ETLEGACY_WOLFADMIN_ARCHIVE "wolfadmin.tar.gz")
set(ETLEGACY_WOLFADMIN_ARCHIVE_URL "https://mirror.etlegacy.com/wolfadmin/wolfadmin.tar.gz")

LEG_DOWNLOAD(
	"WolfAdmin archive"
	"${ETLEGACY_WOLFADMIN_ARCHIVE_URL}"
	"${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${ETLEGACY_WOLFADMIN_ARCHIVE}"
	FALSE
	"${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}"
	"${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/wolfadmin"
)

file(COPY "${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/wolfadmin/luascripts"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}"
)
file(COPY "${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/wolfadmin/lualibs"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}"
)
file(GLOB ETLEGACY_WOLFADMIN_CONFIG
	"${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/wolfadmin/config/*.toml"
)
foreach(ETLEGACY_WOLFADMIN_CONFIG ${ETLEGACY_WOLFADMIN_CONFIG})
	file(COPY "${ETLEGACY_WOLFADMIN_CONFIG}"
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}"
	)
endforeach()

message(STATUS "Adding WolfAdmin to installer scripts")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/luascripts"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/lualibs"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
)
foreach(ETLEGACY_WOLFADMIN_CONFIG ${ETLEGACY_WOLFADMIN_CONFIG})
	install(FILES "${ETLEGACY_WOLFADMIN_CONFIG}"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	)
endforeach()
