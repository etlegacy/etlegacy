#!/bin/sh

#
# This script automatically updates translation template from
# the source code of ET: Legacy
#

# CLIENT
touch etlegacy_client.pot
find src/server/ src/client/ src/qcommon/ -name '*.c' -o -name '*.cpp' | sort | xgettext \
\--from-code=UTF-8 --package-name="ET: Legacy" --copyright-holder="Jan Simek" --package-version="2.70rc2" --msgid-bugs-address="bugs@etlegacy.com" -o etlegacy_client.pot -k_ -k__ -kCL_TranslateString -f -

# MOD
touch etlegacy_mod.pot
find src/game/ src/ui/ src/cgame/ -name '*.c' -o -name '*.cpp' -o -name 'missing_translations_*.txt' | sort | xgettext \
\--from-code=UTF-8 --package-name="ET: Legacy" --copyright-holder="Jan Simek" --package-version="2.70rc2" --msgid-bugs-address="bugs@etlegacy.com" -o etlegacy_mod.pot -k_ -k__ -kCG_TranslateString -ktrap_TranslateString -kTRANSLATE -f -

# TODO: Menu files