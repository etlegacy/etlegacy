#-----------------------------------------------------------------
# Install
#-----------------------------------------------------------------

# description file - see FS_GetModList
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/description.txt"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# misc/etmain/ adds
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/misc/etmain/"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
)

# misc adds
if(INSTALL_OMNIBOT AND UNIX)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etl_bot.sh"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etlded_bot.sh"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endif(INSTALL_OMNIBOT AND UNIX)

# other adds
if(UNIX)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etl.svg"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/com.etlegacy.ETLegacy.desktop"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etlegacy.xml"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/mime/packages"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/com.etlegacy.ETLegacy.metainfo.xml"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/metainfo"
	)
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/linux/man/man6/"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man6"
	)
endif(UNIX)

# project adds
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/docs/INSTALL.txt"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}"
)

# copy required genuine files
if(ET_FS_BASEPATH AND INSTALL_DEFAULT_BASEDIR)
	message(STATUS "Installing genuine W:ET files")

	install(FILES "${ET_FS_BASEPATH}/etmain/pak0.pk3"
		DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	)

	# personal data (owner only)
	install(FILES "${ET_FS_BASEPATH}/etmain/video/etintro.roq"
		DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain/video"
		PERMISSIONS OWNER_WRITE OWNER_READ
	)

	if(ET_KEY)
		install(FILES "${ET_FS_BASEPATH}/etmain/etkey"
			DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain"
			PERMISSIONS OWNER_WRITE OWNER_READ
		)
	endif(ET_KEY)
elseif(NOT ET_FS_BASEPATH AND INSTALL_DEFAULT_BASEDIR)
	message(STATUS "***********************************************************")
	message(STATUS "Genuine W:ET files are not copied - ET: Legacy won't start!")
	message(STATUS "In order to start the game, copy the pak0.pk3 assets file")
	message(STATUS "to ${INSTALL_DEFAULT_BASEDIR}/etmain")
	message(STATUS "***********************************************************")
endif()

# Uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/Uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/Uninstall.cmake"
	IMMEDIATE @ONLY
)
add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Uninstall.cmake
)
