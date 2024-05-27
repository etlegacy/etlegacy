#!/bin/sh

#
# This script automatically updates translation template from
# the source code of ET: Legacy
#

CLIENT_POT=etmain/locale/etlegacy_client.pot
MOD_POT=etmain/locale/etlegacy_mod.pot
VERSION=$(git describe --abbrev=0 --tags)

# CLIENT
touch "${CLIENT_POT}"
find src/server/ src/client/ src/qcommon/ -name '*.c' -o -name '*.cpp' | sort | xgettext \
    --from-code=UTF-8 \
    --package-name="ET: Legacy" --package-version="${VERSION}" \
    --copyright-holder="ET: Legacy Team" --msgid-bugs-address="mail@etlegacy.com" \
    -o "${CLIENT_POT}" -k_ -k__ -kCL_TranslateString -f -

# MOD
touch "${MOD_POT}"
find src/game src/ui src/cgame etmain/ui -name '*.c' -o -name '*.cpp' -o -name '*.menu' | sort | xgettext \
    --language=C --from-code=UTF-8 \
    --package-name="ET: Legacy" --package-version="${VERSION}" \
    --copyright-holder="ET: Legacy Team" --msgid-bugs-address="mail@etlegacy.com" \
    -o "${MOD_POT}" -k_ -k__ -kCG_TranslateString -ktrap_TranslateString -kTRANSLATE -f -

# Update po files
find etmain/locale/mod    -type f -name "*.po" -exec msgmerge -vU {} "${MOD_POT}"    \;
find etmain/locale/client -type f -name "*.po" -exec msgmerge -vU {} "${CLIENT_POT}" \;
