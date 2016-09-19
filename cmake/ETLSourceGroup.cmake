#-----------------------------------------------------------------
# Source Group (for Visual Studio and win32 definitions)
#-----------------------------------------------------------------

if(MSVC OR XCODE)
	# Group the files based on their source path
	SET(SRC_PATH "src")
	FILE(GLOB ALL_SOURCES
		"${SRC_PATH}/*.*"
		"${SRC_PATH}/*/*.*"
	)
	GET_FILENAME_COMPONENT(SRC_FULLPATH ${SRC_PATH} ABSOLUTE)
	FOREACH (SRCFILE ${ALL_SOURCES})
		GET_FILENAME_COMPONENT(FILE_NAME ${SRCFILE} NAME_WE)
		GET_FILENAME_COMPONENT(FILE_FOLDER ${SRCFILE} PATH)
		GET_FILENAME_COMPONENT(FILE_EXT ${SRCFILE} EXT)

		string(REPLACE "${SRC_FULLPATH}/" "" FILE_FOLDER "${FILE_FOLDER}")
		string(REPLACE "${SRC_FULLPATH}" "" FILE_FOLDER "${FILE_FOLDER}")
		string(REPLACE "/" "\\" SHAD_FOLDER_WIN "${FILE_FOLDER}")

		if(FILE_EXT STREQUAL ".cpp" OR FILE_EXT STREQUAL ".c" OR FILE_EXT STREQUAL ".rc")
			source_group("Source Files\\${SHAD_FOLDER_WIN}" FILES ${SRCFILE})
		elseif(FILE_EXT STREQUAL ".hpp" OR FILE_EXT STREQUAL ".h")
			source_group("Header Files\\${SHAD_FOLDER_WIN}" FILES ${SRCFILE})
		endif()
	ENDFOREACH(SRCFILE)
endif()
