#!/bin/sh
# Simple script to start ET Legacy client/listen server
#
# There is no benefit if you don't start with 'fs_game legacy'!
#
./etlded +set fs_game legacy +set omnibot_enable 1 set +omnibot_path "./legacy/omni-bot0.82" +exec etl_server.cfg
