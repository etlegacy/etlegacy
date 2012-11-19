#!/bin/bash

# script checks for needed applications
# and builds et legacy

einfo() {
	echo -e "\n\033[1;32m~~>\033[0m \033[1;37m${1}\033[0m"
}

ehead() {
	echo -e "\033[1;36m * \033[1;37m${1}\033[0m"
}

checkapp() {
	if [ ${2} ]; then
		ISPROBLEM=${2}
	else
		ISPROBLEM=1
	fi

	BINPATH=`which $1 2>/dev/null`

	if [ $? == 0 ]; then
		printf "  %-8s \033[1;32m%s\033[0m: %s\n" "${1}" "found" "${BINPATH}"
	else
		if [ ${ISPROBLEM} == 0 ]; then
			printf "  %-8s \033[1;33m%s\033[0m\n" "${1}" "not found but no problem"
		else
			printf "  %-8s \033[1;31m%s\033[0m\n" "${1}" "not found"
		fi
	fi
}

_detectlinuxdistro() {
	# check for most popular distro files
	_DISTROFILES=(
		"/etc/gentoo-release"
		"/etc/debian_version" "/etc/debian_release" "/etc/ubuntu-release"
		"/etc/redhat-release" "/etc/centos-release" "/etc/fedora-release"
		"/etc/slackware-release"
		"/etc/SuSE-release" "/etc/novell-release" "/etc/sles-release"
	)
	for distro in ${_DISTROFILES}; do
		[ -e "${distro}" ] && echo $(<${distro}) && exit
	done

	# archlinux has empty file...
	[ -e "/etc/arch-release" ] && echo "Arch Linux" && exit

	# oh, maybe we have /etc/lsb-release?
	if [ -e "/etc/lsb-release" ]; then
		. "/etc/lsb-release"
		[ ! ${DISTRIB_DESCRIPTION} ] && DISTRIB_DESCRIPTION="${DISTRIB_ID} ${DISTRIB_RELEASE}"
		echo "${DISTRIB_DESCRIPTION}"
		exit
	fi
	
	echo "Unknown Linux"
}

detectos() {
	PLATFORMSYS=`uname -s`
	PLATFORMARCH=`uname -m`
	if [[ ${PLAFORMSYS} -eq "Linux" ]]; then
		DISTRO=`_detectlinuxdistro`
	else
		DISTRO=""
	fi
	echo -e "  running on: \033[1;32m${PLATFORMSYS}\033[0m \033[0;32m${PLATFORMARCH}\033[0m - \033[1;36m${DISTRO}\033[0m"
}

_SRC=`pwd`
BUILDDIR="${_SRC}/build"

echo
ehead "ET Legacy Easy Builder"
ehead "==============================="
ehead "This script will check for binaries needed to compile ET Legacy"
ehead "Then it'll build ET Legacy into build/ directory"
echo

einfo "Checking for needed apps to compile..."

echo
detectos
echo
checkapp autoconf
checkapp cmake
checkapp gcc
checkapp g++
checkapp clang 0
checkapp clang++ 0
checkapp git
checkapp zip
echo

# everything looks ok, try to compile!

# cmake varialbes
BUILD_CLIENT=1
BUILD_OMNIBOT=1
BUNDLED_LIBS=1
FEATURE_OGG_VORBIS=0
FEATURE_TRACKBASE=1

mkdir -p ${BUILDDIR}
if [ -e "${_SRC}/libs/CMakeLists.txt" ]; then
	einfo "Cleaning SDL..."
	cd ${_SRC}/libs/sdl;  make clean
	einfo "Cleaning libjpeg..."
	cd ${_SRC}/libs/jpeg; make clean
else
	einfo "Getting bundled libs..."
	git submodule init
	git submodule update
fi

cd ${BUILDDIR}
einfo "Configuring ET Legacy..."
_CFGSTRING="
	-DBUILD_CLIENT=${BUILD_CLIENT}
	-DBUILD_SERVER=1
	-DBUILD_MOD=1
	-DBUILD_MOD_PK3=1
	-DBUILD_PAK3_PK3=1
	-DBUILD_OMNIBOT=${BUILD_OMNIBOT}
	-DBUNDLED_LIBS=${BUNDLED_LIBS}
	-DCROSS_COMPILE32=1
	-DFEATURE_CURL=1
	-DFEATURE_OGG_VORBIS=${FEATURE_OGG_VORBIS}
	-DFEATURE_FREETYPE=0
	-DFEATURE_OPENAL=0
	-DFEATURE_TRACKBASE=${FEATURE_TRACKBASE}
"

echo -e "\033[1;33musing: \033[1;37m${_CFGSTRING}\033[0m"
cmake ${_CFGSTRING} ..

einfo "Compiling ET Legacy..."
make ${MAKEOPTS}
