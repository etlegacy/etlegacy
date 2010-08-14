#!/bin/sh

# I've found that XCode's stripping capabilities are sorely lacking
# plus, it used to copy the game/cgame/ui to the Resource folder
# but it can't copy the right Deployment/Development depending on configuration type
# (or I couldn't get it to)

root=`pwd`

# so this script copies and strips.. (Deployment binary)
(
cd build/Deployment

cp -Rv cgame_mac.bundle 'Wolfenstein ET.app/Contents/Resources'
cp -Rv ui_mac.bundle 'Wolfenstein ET.app/Contents/Resources'
cp -Rv qagame_mac.bundle 'Wolfenstein ET.app/Contents/Resources'

(
cd 'Wolfenstein ET.app/Contents/Resources'
echo "Stripping the binaries"
strip -u -s $root/exports.def cgame_mac.bundle/Contents/MacOS/cgame_mac
strip -u -s $root/exports.def qagame_mac.bundle/Contents/MacOS/qagame_mac
strip -u -s $root/exports.def ui_mac.bundle/Contents/MacOS/ui_mac
)
)

echo 'MAKE SURE YOU STRIP THE MAIN BINARY AS WELL (strip -u -r)'
