#-----------------------------------------------------------------
# Install OmniBot
#-----------------------------------------------------------------

message(STATUS "Installing Omni-Bot")
# Note: used archive (20MB) doesn't contain incomplete nav- and other unwanted files
if(UNIX)
	set(ETLEGACY_OMNIBOT_ARCHIVE "omnibot-linux-latest.tar.gz")
	set(ETLEGACY_OMNIBOT_ARCHIVE_URL "http://mirror.etlegacy.com/omnibot/omnibot-linux-latest.tar.gz")
elseif(WIN32 AND UNZIP_EXECUTABLE)
	set(ETLEGACY_OMNIBOT_ARCHIVE "omnibot-windows-latest.zip")
	set(ETLEGACY_OMNIBOT_ARCHIVE_URL "http://mirror.etlegacy.com/omnibot/omnibot-windows-latest.zip")
endif()

set(ETLEGACY_OMNIBOT_DL_URL "${ETLEGACY_OMNIBOT_ARCHIVE_URL}")

#file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy")

message(STATUS "Downloading Omni-Bot archive to ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}")
	file(DOWNLOAD
		${ETLEGACY_OMNIBOT_DL_URL}
		"${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}"
		SHOW_PROGRESS TIMEOUT 30
	)
endif()

message(STATUS "Extracting Omni-Bot to ${CMAKE_CURRENT_BINARY_DIR}/legacy/omni-bot")
if(UNIX)
	execute_process(
		COMMAND tar -xf ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}
		COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
	)
elseif(WIN32)
	execute_process(
		COMMAND ${UNZIP_EXECUTABLE} -u ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
	)
endif()

message(STATUS "Adding Omni-Bot to installer scripts")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy/omni-bot/"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy/omni-bot"
)

# ensure unique permissions and bot user path has write permission set
if(UNIX)
	execute_process(
		COMMAND chmod -R 644 "${INSTALL_DEFAULT_MODDIR}/legacy/omni-bot/"
		WORKING_DIRECTORY "${INSTALL_DEFAULT_MODDIR}/legacy"
	)
endif(UNIX)
