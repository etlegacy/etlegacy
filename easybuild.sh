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
	_DISTROFILES="
		/etc/gentoo-release /etc/slackware-release
		/etc/debian_version /etc/debian_release /etc/ubuntu-release
		/etc/redhat-release /etc/centos-release /etc/fedora-release
		/etc/SuSE-release /etc/novell-release /etc/sles-release"

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
	if [[ ${PLATFORMSYS} == "Linux" ]]; then
		DISTRO=`_detectlinuxdistro`
	elif [[ ${PLATFORMSYS} == "Darwin" ]]; then
		PLATFORMSYS=`sw_vers -productName`
		DISTRO=`sw_vers -productVersion`
	else
		DISTRO="Unknown"
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
[ ! "${RELEASE_TYPE}" ]    && RELEASE_TYPE="Release"
[ ! "${CROSS_COMPILE32}" ] && CROSS_COMPILE32=1

[ ! "${BUILD_CLIENT}" ] && BUILD_CLIENT=1
[ ! "${BUNDLED_LIBS}" ] && BUNDLED_LIBS=1
[ ! "${BUNDLED_SDL}" ]  && BUNDLED_SDL=1
[ ! "${BUNDLED_JPEG}" ] && BUNDLED_JPEG=1
[ ! "${BUNDLED_LUA}" ]  && BUNDLED_LUA=1
[ ! "${BUNDLED_CURL}" ] && BUNDLED_CURL=1
[ ! "${BUNDLED_OGG}" ]  && BUNDLED_OGG=1
FEATURE_OGG=1
FEATURE_TRACKER=1
FEATURE_OMNIBOT=1
[ ! "${FEATURE_ANTICHEAT}" ] && FEATURE_ANTICHEAT=0
FEATURE_LUA=1

mkdir -p ${BUILDDIR}
CLEANLIBS=0
if [[ -e "${_SRC}/libs/CMakeLists.txt" && ${CLEANLIBS} ]]; then
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
	-DCMAKE_BUILD_TYPE=${RELEASE_TYPE}
	-DBUILD_CLIENT=${BUILD_CLIENT}
	-DBUILD_SERVER=1
	-DBUILD_MOD=1
	-DBUILD_MOD_PK3=1
	-DBUILD_PAK3_PK3=1
	-DBUNDLED_LIBS=${BUNDLED_LIBS}
	-DBUNDLED_SDL=${BUNDLED_SDL}
	-DBUNDLED_JPEG=${BUNDLED_JPEG}
	-DBUNDLED_LUA=${BUNDLED_LUA}
	-DBUNDLED_CURL=${BUNDLED_CURL}
	-DBUNDLED_OGG_VORBIS=${BUNDLED_OGG}
	-DCROSS_COMPILE32=${CROSS_COMPILE32}
	-DFEATURE_CURL=1
	-DFEATURE_OGG_VORBIS=${FEATURE_OGG}
	-DFEATURE_FREETYPE=0
	-DFEATURE_OPENAL=0
	-DFEATURE_TRACKER=${FEATURE_TRACKER}
	-DFEATURE_OMNIBOT=${FEATURE_OMNIBOT}
	-DFEATURE_ANTICHEAT=${FEATURE_ANTICHEAT}
	-DFEATURE_LUA=${FEATURE_LUA}
"

if [ "${DEV}" != 1 ]; then
	_CFGSTRING="${_CFGSTRING}
	-DINSTALL_DEFAULT_MODDIR=${HOME}/etlegacy
	-DINSTALL_DEFAULT_BINDIR=${HOME}/etlegacy
	-DINSTALL_DEFAULT_BASEDIR=${HOME}/etlegacy
"
fi

echo -e "\033[1;33musing: \033[1;37m${_CFGSTRING}\033[0m"
cmake ${_CFGSTRING} ..

einfo "Compiling ET Legacy..."
make ${MAKEOPTS}
