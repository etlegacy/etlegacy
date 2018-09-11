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
	"${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}"
	"ddc91eb5a0a0a245d29cb3a6038c43ef"
	"${CMAKE_CURRENT_BINARY_DIR}/legacy"
	"${CMAKE_CURRENT_BINARY_DIR}/legacy/omni-bot"
)

message(STATUS "Adding Omni-Bot to installer scripts")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy/omni-bot/"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy/omni-bot"
	DIRECTORY_PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ WORLD_READ
)
