# This file will have the dummy and real targets setup

# Shared libraries (client, server and mod bins)
add_library(shared_libraries INTERFACE)

# All client only libraries
add_library(client_libraries INTERFACE)

# All server only libraries
add_library(server_libraries INTERFACE)

# Engine common libraries (both client and engine have these)
add_library(engine_libraries INTERFACE)

# All renderer common libraries
add_library(renderer_libraries INTERFACE)

add_library(renderer_gl1_libraries INTERFACE)
add_library(renderer_gl2_libraries INTERFACE)
add_library(renderer_gles_libraries INTERFACE)
add_library(renderer_vulkan_libraries INTERFACE)

# Mod shared libraries
add_library(mod_libraries INTERFACE)

# CGame mod libraries
add_library(cgame_libraries INTERFACE)

# UI mod libraries
add_library(ui_libraries INTERFACE)

# Server mod libraries
add_library(qagame_libraries INTERFACE)
add_library(tvgame_libraries INTERFACE)

# JSON library helper target
add_library(etl_json INTERFACE)
target_sources(etl_json INTERFACE "${PROJECT_SOURCE_DIR}/src/qcommon/json/json.c")
target_include_directories(etl_json INTERFACE "${PROJECT_SOURCE_DIR}/src/qcommon/json/")

# Link the shared libraries to all output bins
target_link_libraries(engine_libraries INTERFACE shared_libraries)
target_link_libraries(renderer_libraries INTERFACE shared_libraries)
target_link_libraries(mod_libraries INTERFACE shared_libraries)
