#-----------------------------------------------------------------
# Install
#-----------------------------------------------------------------

if(NOT DOCDIR)
	set(DOCDIR "${INSTALL_DEFAULT_SHAREDIR}/doc/etlegacy")
endif()

# description file - see FS_GetModList
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/description.txt"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

# misc/etmain/ adds
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/misc/etmain/"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
)

# misc adds
if(INSTALL_OMNIBOT AND UNIX)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/misc/etl_bot.sh.in"
		       "${CMAKE_CURRENT_BINARY_DIR}/misc/etl_bot${BIN_SUFFIX}.sh" @ONLY)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/misc/etlded_bot.sh.in"
		       "${CMAKE_CURRENT_BINARY_DIR}/misc/etlded_bot${BIN_SUFFIX}.sh" @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/misc/etl_bot${BIN_SUFFIX}.sh"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/misc/etlded_bot${BIN_SUFFIX}.sh"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endif()

# other adds
if(UNIX AND NOT APPLE)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/misc/com.etlegacy.ETLegacy.desktop.in"
		       "${CMAKE_CURRENT_BINARY_DIR}/misc/com.etlegacy.ETLegacy${BIN_SUFFIX}.desktop" @ONLY)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/misc/com.etlegacy.ETLegacy.metainfo.xml.in"
			"${CMAKE_CURRENT_BINARY_DIR}/misc/com.etlegacy.ETLegacy.metainfo.xml" @ONLY)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/misc/etlegacy.service.in"
		       "${CMAKE_CURRENT_BINARY_DIR}/misc/etlegacy${BIN_SUFFIX}.service" @ONLY)

	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etl.svg"
		DESTINATION "${INSTALL_DEFAULT_SHAREDIR}/icons/hicolor/scalable/apps"
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/misc/com.etlegacy.ETLegacy${BIN_SUFFIX}.desktop"
		DESTINATION "${INSTALL_DEFAULT_SHAREDIR}/applications"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etlegacy.xml"
		DESTINATION "${INSTALL_DEFAULT_SHAREDIR}/mime/packages"
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/misc/com.etlegacy.ETLegacy.metainfo.xml"
		DESTINATION "${INSTALL_DEFAULT_SHAREDIR}/metainfo"
	)
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/linux/man/man6/"
		DESTINATION "${INSTALL_DEFAULT_SHAREDIR}/man/man6"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/docs/INSTALL.txt"
		DESTINATION "${DOCDIR}"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt"
		DESTINATION "${INSTALL_DEFAULT_SHAREDIR}/licenses/etlegacy"
	)
else()
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/docs/INSTALL.txt"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
	)
endif()

if(APPLE)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/macOS-packs.command"
		DESTINATION "${INSTALL_DEFAULT_BASEDIR}/"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endif()

# copy required genuine files
if(ET_FS_BASEPATH AND INSTALL_DEFAULT_BASEDIR)
	message(STATUS "Installing genuine W:ET files")

	install(FILES "${ET_FS_BASEPATH}/etmain/pak0.pk3"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/pak1.pk3"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/pak2.pk3"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	)

	# personal data (owner only)
	install(FILES "${ET_FS_BASEPATH}/etmain/video/etintro.roq"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain/video"
		PERMISSIONS OWNER_WRITE OWNER_READ
	)

	if(ET_KEY)
		install(FILES "${ET_FS_BASEPATH}/etmain/etkey"
			DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
			PERMISSIONS OWNER_WRITE OWNER_READ
		)
	endif(ET_KEY)
elseif(NOT ET_FS_BASEPATH AND INSTALL_DEFAULT_BASEDIR)
	message(STATUS "***********************************************************")
	message(STATUS "Genuine W:ET files are not copied - ET: Legacy won't start!")
	message(STATUS "In order to start the game, copy the pak0-2.pk3 asset files")
	message(STATUS "to ${INSTALL_DEFAULT_MODDIR}/etmain")
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
