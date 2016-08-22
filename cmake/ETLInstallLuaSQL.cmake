#-----------------------------------------------------------------
# Build and Install LuaSQL - SQLite3
#-----------------------------------------------------------------

message(STATUS "Installing LuaSQL with SQLite3 backend")

if(NOT BUNDLED_LUA)
	set(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIR})
	set(LUA_LIBRARIES ${LUA_LIBRARIES})
else() # BUNDLED_LUA
	set(LUA_INCLUDE_DIR ${LUA_BUNDLED_INCLUDE_DIR})
	set(LUA_LIBRARIES ${LUA_BUNDLED_LIBRARIES})
endif(NOT BUNDLED_LUA)

if(NOT BUNDLED_SQLITE3)
	set(SQLITE3_INCLUDE_DIR ${SQLITE3_INCLUDE_DIR})
	set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARIES})
else() # BUNDLED_SQLITE3
	set(SQLITE3_INCLUDE_DIR ${SQLITE3_BUNDLED_INCLUDE_DIR})
	set(SQLITE3_LIBRARIES ${SQLITE3_BUNDLED_LIBRARIES})
endif(NOT BUNDLED_SQLITE3)

set(LUASQL_DIR "src/luasql")

FILE(GLOB LUASQL_FILES
	"${LUASQL_DIR}/luasql.c"
	"${LUASQL_DIR}/luasql.h"
)

FILE(GLOB LUASQL_BACKEND_FILES
	"${LUASQL_DIR}/ls_sqlite3.c"
)

set(LUASQL_PUBLIC_HEADERS ${LUASQL_DIR}/luasql.h)

include_directories(${LUA_INCLUDE_DIR})
include_directories(${SQLITE3_INCLUDE_DIR})
include_directories("${LUASQL_DIR}")

add_library(luasql_library_module MODULE ${LUASQL_PUBLIC_HEADERS} ${LUASQL_FILES} ${LUASQL_BACKEND_FILES})
set_target_properties(luasql_library_module PROPERTIES
	C_VISIBILITY_PRESET default
	VISIBILITY_INLINES_HIDDEN 0
	PREFIX ""
	OUTPUT_NAME "sqlite3"
)

target_link_libraries(luasql_library_module
	${SQLITE3_LIBRARIES}
	${LUA_LIBRARIES}
)

if(BUNDLED_LUA)
	add_dependencies(luasql_library_module bundled_lua bundled_sqlite3)
endif(BUNDLED_LUA)
add_dependencies(luasql_library_module qagame${LIB_SUFFIX}${ARCH})

if(NOT BUNDLED_LUA)
	# get lua library exact name
	string(REPLACE "/" ";" LUA_LIBRARY_LIST ${LUA_LIBRARY})
	list(GET LUA_LIBRARY_LIST "-1" LUA_LIBRARY_NAME)
	# symlink
	execute_process(
		COMMAND "${CMAKE_COMMAND}" "-E" "create_symlink" "${LUA_LIBRARY}.${LUA_VERSION_STRING}" "./${LUA_LIBRARY_NAME}.${LUA_VERSION_STRING}"
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${LUA_LIBRARY_NAME}.${LUA_VERSION_STRING}" DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy/lualibs/luasql")
endif(NOT BUNDLED_LUA)
