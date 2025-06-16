#!/bin/bash
set -e

# Optional fs_game support
FS_GAME_PARAM=""
if [ -n "$FS_GAME" ]; then
    FS_GAME_PARAM="+set fs_game $FS_GAME"
fi

exec ./etlded +set fs_homepath /legacy/homepath $FS_GAME_PARAM +exec etl_server.cfg