#!/bin/sh
# Simple script to start 32-bit ET: Legacy dedicated server with Omni-Bots
#
./etlded.x86 +set g_protect 1 +set omnibot_enable 1 +set omnibot_path "./legacy/omni-bot" +exec etl_server.cfg
