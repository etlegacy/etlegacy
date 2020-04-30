#-----------------------------------------------------------------
# Install OmniBot
#-----------------------------------------------------------------

message(STATUS "Installing Omni-Bot")
# Note: used archive (20MB) doesn't contain incomplete nav- and other unwanted files
if(UNIX)
	set(ETLEGACY_OMNIBOT_ARCHIVE "omnibot-linux-latest.tar.gz")
	set(ETLEGACY_OMNIBOT_ARCHIVE_URL "https://mirror.etlegacy.com/omnibot/omnibot-linux-latest.tar.gz")
elseif(WIN32)
	set(ETLEGACY_OMNIBOT_ARCHIVE "omnibot-windows-latest.zip")
	set(ETLEGACY_OMNIBOT_ARCHIVE_URL "https://mirror.etlegacy.com/omnibot/omnibot-windows-latest.zip")
endif()

set(ETLEGACY_OMNIBOT_DL_URL "${ETLEGACY_OMNIBOT_ARCHIVE_URL}")

#file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy")

LEG_DOWNLOAD(
	"Omni-Bot archive"
	${ETLEGACY_OMNIBOT_DL_URL}
	"${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${ETLEGACY_OMNIBOT_ARCHIVE}"
	FALSE
	"${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}"
	"${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/omni-bot"
)

message(STATUS "Adding Omni-Bot to installer scripts")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/omni-bot/"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}/omni-bot"
	DIRECTORY_PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ WORLD_READ
)
