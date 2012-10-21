#!/bin/sh
# Creates Mac OS X bundle
# Execute this script from the build directory

LIBRARIES=( ui cgame qagame )
for LIB in "${LIBRARIES[@]}"
do

if [ ! -f ./etmain/${LIB}_mac ] ; then
  echo "Missing ./etmain/${LIB}_mac"
  exit
fi

echo "Creating ${LIB}_mac.bundle"

rm -rf ${LIB}_mac.bundle
mkdir -p ${LIB}_mac.bundle/Contents/MacOS
cp ./etmain/${LIB}_mac ${LIB}_mac.bundle/Contents/MacOS/${LIB}_mac

cat << EOF > ${LIB}_mac.bundle/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
        <key>CFBundleDevelopmentRegion</key>
        <string>English</string>
        <key>CFBundleExecutable</key>
        <string>${LIB}_mac</string>
        <key>CFBundleInfoDictionaryVersion</key>
        <string>6.0</string>
        <key>CFBundleName</key>
        <string>${LIB}</string>
        <key>CFBundlePackageType</key>
        <string>BNDL</string>
        <key>CFBundleShortVersionString</key>
        <string>1.01c</string>
        <key>CFBundleSignature</key>
        <string>JKAm</string>
        <key>CFBundleVersion</key>
        <string>1.01c</string>
        <key>CFPlugInDynamicRegisterFunction</key>
        <string></string>
        <key>CFPlugInDynamicRegistration</key>
        <string>NO</string>
        <key>CFPlugInFactories</key>
        <dict>
                <key>00000000-0000-0000-0000-000000000000</key>
                <string>MyFactoryFunction</string>
        </dict>
        <key>CFPlugInTypes</key>
        <dict>
                <key>00000000-0000-0000-0000-000000000000</key>
                <array>
                        <string>00000000-0000-0000-0000-000000000000</string>
                </array>
        </dict>
        <key>CFPlugInUnloadFunction</key>
        <string></string>
</dict>
</plist>
EOF

rm -f ${LIB}_mac.zip
#try 7-zip first, then pkzip
7za > /dev/null 2>&1
if [ "$?" = "0" ] ; then
  7za a -r -tzip -mx=9 -mpass=4 -mfb=255 ${LIB}_mac.zip ${LIB}_mac.bundle
else
  zip -r9 ${LIB}_mac.zip ${LIB}_mac.bundle
fi
if [ "$?" != "0" ] ; then
  echo "Error - couldn't create zipfile"
else
  mv ${LIB}_mac.zip ./etmain/${LIB}_mac
fi

done
