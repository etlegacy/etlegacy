#-----------------------------------------------------------------
# Build Pack
#-----------------------------------------------------------------

#
# pak3.pk3
#
file(GLOB etmain_files_in "${CMAKE_CURRENT_SOURCE_DIR}/etmain/*")
FOREACH (file ${etmain_files_in})
    file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}/etmain" ${file})
    list(APPEND etmain_files_pack ${rel})
ENDFOREACH()

add_custom_target(
	pak3_pk3 ALL
	COMMAND ${CMAKE_COMMAND} -E tar c "${CMAKE_CURRENT_BINARY_DIR}/legacy/pak3_${ETL_CMAKE_VERSION_SHORT}.pk3" --format=zip ${etmain_files_pack}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/etmain/
	VERBATIM
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/legacy/pak3_${ETL_CMAKE_VERSION_SHORT}.pk3
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
)
