#!/usr/bin/env bash

set -Eeuo pipefail
trap cleanup SIGINT SIGTERM ERR EXIT
dir=$(pwd)

cleanup() {
	trap - SIGINT SIGTERM ERR EXIT
	cd "$dir"
}

setup_colors() {
	if [[ -t 2 ]] && [[ -z "${NO_COLOR-}" ]] && [[ "${TERM-}" != "dumb" ]]; then
		NOFORMAT='\033[0m' RED='\033[0;31m' GREEN='\033[0;32m' ORANGE='\033[0;33m' BLUE='\033[0;34m' PURPLE='\033[0;35m' CYAN='\033[0;36m' YELLOW='\033[1;33m'
	else
		NOFORMAT='' RED='' GREEN='' ORANGE='' BLUE='' PURPLE='' CYAN='' YELLOW=''
	fi
}

msg() {
	echo >&2 -e "${1-}"
}

die() {
	local msg=$1
	local code=${2-1} # default exit status 1
	msg "$msg"
	exit "$code"
}

setup_colors

source_folder="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
wolf_et_dmg=$source_folder/WolfET.2.60d.dmg

if [ -f "$wolf_et_dmg" ]; then
	msg "${YELLOW}$wolf_et_dmg ${GREEN}exists.${NOFORMAT}"
	temp_dir=$(mktemp -d)
	hdiutil attach "$wolf_et_dmg" -private -noverify -nobrowse -noautoopen -quiet -mountpoint "$temp_dir/et"
	msg "Copying files paks to: ~/Library/Application Support/etlegacy/etmain"
	cp $temp_dir/et/Wolfenstein\ ET/etmain/{pak0.pk3,pak1.pk3,pak2.pk3} ~/Library/Application\ Support/etlegacy/etmain/
	msg "Unmounting the original image"
	hdiutil unmount "$temp_dir/et"
	msg "Removing the temporary folder"
	rm -Rf "$temp_dir"
else
	temp_dir=$(mktemp -d)
	msg "${YELLOW}$wolf_et_dmg ${RED}does not exist downloading.${NOFORMAT}"
	wolf_et_dmg=$temp_dir/WolfET.2.60d.dmg
	msg "Downloading to: $wolf_et_dmg"
	curl --fail -LJ -o $wolf_et_dmg https://cdn.splashdamage.com/downloads/games/wet/WolfET.2.60d.dmg || curl --fail -LJ -o $wolf_et_dmg https://mirror.etlegacy.com/vanilla-files/WolfET.2.60d.dmg
	hdiutil attach "$wolf_et_dmg" -private -noverify -nobrowse -noautoopen -quiet -mountpoint "$temp_dir/et"
	mkdir -p ~/Library/Application\ Support/etlegacy/etmain/
	msg "Copying files paks to: ~/Library/Application Support/etlegacy/etmain"
	cp $temp_dir/et/Wolfenstein\ ET/etmain/{pak0.pk3,pak1.pk3,pak2.pk3} ~/Library/Application\ Support/etlegacy/etmain/
	msg "Unmounting the original image"
	hdiutil unmount "$temp_dir/et"
	msg "Removing the temporary folder"
	rm -Rf "$temp_dir"
fi

msg "${GREEN}All done!${NOFORMAT}"
