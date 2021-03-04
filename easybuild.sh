#!/usr/bin/env bash
# encoding: utf-8

# Made by the ET: Legacy team!
# script checks for needed applications
# and builds ET: Legacy

# Mandatory variables
_SRC=`pwd`
#BUILDDIR="${_SRC}/build"
BUILDDIR="${BUILD_DIR:-${_SRC}/build}"
SOURCEDIR="${_SRC}/src"
PROJECTDIR="${_SRC}/project"

if [[ `uname -s` == "Darwin" ]]; then
	MODMAIN="${HOME}/Library/Application Support/etlegacy/etmain"
else
	MODMAIN="${HOME}/.etlegacy/etmain"
fi

ETLEGACY_MIRROR="https://mirror.etlegacy.com/etmain/"
ETLEGACY_VERSION=`git describe --abbrev=7 2>/dev/null`
INSTALL_PREFIX=${HOME}/etlegacy

# Set this to false to disable colors
color=true

# Do 32bit build
x86_build=true


# Command that can be run
# first array has the cmd names which can be given
# second array holds the functions which match the cmd names
easy_keys=(clean build generate package install download crust release project help)
easy_cmd=(run_clean run_build run_generate run_package run_install run_download run_uncrustify run_release run_project print_help)
easy_count=`expr ${#easy_keys[*]} - 1`

check_exit() {
	EXIT_CODE=$?
	if [ "$#" -ne 0 ]; then
		eval "$1"
		EXIT_CODE=$?
	fi
	if [ $EXIT_CODE != 0 ]; then
		echo Exiting!
		exit $EXIT_CODE
	fi
}

if $color; then
	# For color codes, see e.g. http://www.cplusplus.com/forum/unices/36461/
	boldgreen='\033[1;32m'
	boldlightblue='\033[1;36m'
	boldwhite='\033[1;37m'
	boldyellow='\033[1;33m'
	boldred='\033[1;31m'
	darkgreen='\033[0;32m'
	reset='\033[0m'
else
	boldgreen=
	boldlightblue=
	boldwhite=
	boldyellow=
	boldred=
	darkgreen=
	reset=
fi

einfo() {
	echo -e "\n$boldgreen~~>$reset $boldwhite${1}$reset"
}

ehead() {
	echo -e "$boldlightblue * $boldwhite${1}$reset"
}

app_exists() {
	local __resultvar=$1
	local app_result=0
	BINPATH=`which $2 2>/dev/null`
	if [ $? == 0 ]; then
		local app_result=1
	fi

	eval $__resultvar="'$app_result'"
}

checkapp() {
	if [ ${2} ]; then
		ISPROBLEM=${2}
	else
		ISPROBLEM=1
	fi

	app_exists APP_FOUND $1

	if [ $APP_FOUND == 1 ]; then
		printf "  %-10s $boldgreen%s$reset: %s\n" "${1}" "found" "${BINPATH}"
	else
		if [ ${ISPROBLEM} == 0 ]; then
			printf "  %-10s $boldyellow%s$reset\n" "${1}" "not found but no problem"
		else
			printf "  %-10s $boldred%s$reset\n" "${1}" "not found"
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

	# Alpine Linux only has a version number
	[ -e "/etc/alpine-release" ] && echo "Alpine Linux $(</etc/alpine-release)" && exit

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

		# Check if x86_build is set and an osx vesion as of Catalina or higher is used
		IFS='.' read -r -a ver <<< "$DISTRO"
		if ([ "${ver[0]}" -gt 10 ] || [ "${ver[1]}" -gt 13 ]) && [ "${x86_build}" = true ]; then
			einfo "You can't compile 32bit binaries with Mac OS ${ver[0]}.${ver[1]}. Use the flag \"-64\". Aborting."
			exit 1
		fi
	else
		DISTRO="Unknown"
	fi
	echo -e "  running on: $boldgreen${PLATFORMSYS}$reset $darkgreen${PLATFORMARCH}$reset - $boldlightblue${DISTRO}$reset"
}

# This is in reference to https://cmake.org/pipermail/cmake/2016-April/063312.html
# Long story short, setting the cross compile state in cmake does not work on all platforms
# so lets set the -m32 flag before we run cmake
set_compiler() {
	if [ ${3} == true ]; then
		export CC="${1} -m32"
		export CXX="${2} -m32"
	else
		export CC=${1}
		export CXX=${2}
	fi
}

check_compiler() {
	if [ -z "$CC" ] && [ -z "$CXX" ]; then
		app_exists GCCFOUND "gcc"
		app_exists GPLUSFOUND "g++"
		app_exists CLANGFOUND "clang"
		app_exists CLANGPLUSFOUND "clang++"
		if [ $GCCFOUND == 1 ] && [ $GPLUSFOUND == 1 ]; then
			set_compiler gcc g++ $x86_build
		elif [ $CLANGFOUND == 1 ] && [ $CLANGPLUSFOUND == 1 ]; then
			set_compiler clang clang++ $x86_build
		else
			einfo "Missing compiler. Exiting."
			exit 1
		fi
	fi
}

print_startup() {
	echo
	ehead "ET: Legacy Easy Builder"
	ehead "==============================="
	ehead "This script will check for binaries needed to compile ET: Legacy"
	ehead "Then it will build ET: Legacy into ${BUILDDIR}/ directory"
	echo

	einfo "Checking for needed apps to compile..."

	echo
	detectos
	echo
	checkapp autoconf
	checkapp libtoolize
	checkapp cmake
	checkapp gcc
	checkapp g++
	checkapp clang 0
	checkapp clang++ 0
	checkapp git
	checkapp zip
	checkapp nasm
	echo

	check_compiler

	einfo "Using compilers:"

	echo
	echo "  CC  = ${CC}"
	echo "  CXX = ${CXX}"
}

setup_sensible_defaults() {
	# Default to 64 bit builds on OSX
	if [[ `uname -s` == "Darwin" ]]; then
		CROSS_COMPILE32=0
		x86_build=false
	fi
}

parse_commandline() {
	for var in "$@"
	do
		if [[ $var == --build=* ]]; then
			BUILDDIR=$(echo $var| cut -d'=' -f 2)
			einfo "Will use build dir of: ${BUILDDIR}"
		elif [[ $var == --prefix=* ]]; then
			INSTALL_PREFIX=$(echo $var| cut -d'=' -f 2)
			einfo "Will use installation dir of: ${INSTALL_PREFIX}"
		elif [[ $var == --osx=* ]]; then
			MACOSX_DEPLOYMENT_TARGET=$(echo $var| cut -d'=' -f 2)
			einfo "Will use OSX target version: ${MACOSX_DEPLOYMENT_TARGET}"
		elif [[ $var == --sysroot=* ]]; then
			XCODE_SDK_PATH=$(echo $var| cut -d'=' -f 2)
			einfo "Will use OSX sysroot: ${XCODE_SDK_PATH}"
		elif [ "$var" = "-64" ]; then
			einfo "Will disable crosscompile"
			CROSS_COMPILE32=0
			x86_build=false
		elif [ "$var" = "-32" ]; then
			einfo "Will enable crosscompile"
			CROSS_COMPILE32=1
			x86_build=true
		elif [ "$var" = "-no-ssl" ]; then
			einfo "Will disable SSL"
			FEATURE_SSL=0
			BUNDLED_OPENSSL=0
		elif [ "$var" = "-zip" ]; then
			ZIP_ONLY=1
		elif [ "$var" = "-clang" ]; then
			einfo "Will use clang"
			set_compiler clang clang++ $x86_build
		elif [ "$var" = "-gcc" ]; then
			einfo "Will use gcc"
			set_compiler gcc g++ $x86_build
		elif [ "$var" = "-debug" ]; then
			einfo "Will enable debug build"
			RELEASE_TYPE="Debug"
		elif [ "$var" = "-nodb" ]; then
			einfo "Will disable database"
			FEATURE_DBMS=0
			BUNDLED_SQLITE3=0
		elif [ "$var" = "-nor2" ]; then
			einfo "Will disable renderer2"
			FEATURE_RENDERER2=0
		elif [ "$var" = "-nodynamic" ]; then
			einfo "Will disable dynamic renderer build"
			RENDERER_DYNAMIC=0
		elif [ "$var" = "-jpeg-turbo" ]; then
			einfo "Will enable system jpeg turbo"
			BUNDLED_JPEG=0
		elif [ "$var" = "-systemlib" ]; then
			einfo "Will disable bundled libraries"
			BUNDLED_LIBS=0
			BUNDLED_SDL=0
			BUNDLED_ZLIB=0
			BUNDLED_MINIZIP=0
			BUNDLED_CURL=0
			BUNDLED_JPEG=0
			BUNDLED_LUA=0
			BUNDLED_OGG_VORBIS=0
			BUNDLED_THEORA=0
			BUNDLED_OPENAL=0
			BUNDLED_GLEW=0
			BUNDLED_FREETYPE=0
			BUNDLED_PNG=0
			BUNDLED_SQLITE3=0
		elif [ "$var" = "-noextra" ]; then
			einfo "Will disable installation of Omni-bot, GeoIP and WolfAdmin"
			INSTALL_EXTRA=0
			INSTALL_OMNIBOT=0
			INSTALL_GEOIP=0
			INSTALL_WOLFADMIN=0
		elif [ "$var" = "-noupdate" ]; then
			einfo "Will disable autoupdate"
			FEATURE_AUTOUPDATE=0
		elif [ "$var" = "-RPI" ]; then
			einfo "Will enable Raspberry PI build ..."
			ARM=1
			CROSS_COMPILE32=0
			x86_build=false
			FEATURE_RENDERER_GLES=0
			RENDERER_DYNAMIC=0
			FEATURE_RENDERER2=0
			# FIXME: ogg doesn't compile
			BUNDLED_OGG_VORBIS=0
			FEATURE_OGG_VORBIS=0
			# FIXME
			FEATURE_THEORA=0
			BUNDLED_THEORA=0
			# not required
			BUNDLED_GLEW=0
			# FIXME: needs -PIC
			FEATURE_FREETYPE=0
			BUNDLED_FREETYPE=0
			#FEATURE_DBMS=0
			#BUNDLED_SQLITE3=0
			FEATURE_LUASQL=1
			FEATURE_PNG=0
			FEATURE_OMNIBOT=1
			INSTALL_OMNIBOT=0
		elif [ "$var" = "-mod" ]; then
			einfo "Will only build the mod"
			BUILD_CLIENT=0
			BUILD_SERVER=0
			FEATURE_RENDERER2=0
			FEATURE_RENDERER_GLES=0
			RENDERER_DYNAMIC=0

			FEATURE_CURL=0
			FEATURE_OGG_VORBIS=0
			FEATURE_THEORA=0
			FEATURE_OPENAL=0
			FEATURE_FREETYPE=0
			FEATURE_PNG=0

			BUNDLED_SDL=0
			# FIXME: this needs to be fixed in cmake, we do not want zlib or minizip if we are not building the client or server
			BUNDLED_ZLIB=1
			BUNDLED_MINIZIP=1
			BUNDLED_JPEG=0
			BUNDLED_OGG_VORBIS=0
			BUNDLED_THEORA=0
			BUNDLED_GLEW=0
			BUNDLED_FREETYPE=0
			BUNDLED_PNG=0
			BUNDLED_CURL=0
			BUNDLED_OPENAL=0
		elif [ "$var" = "-server" ]; then
			einfo "Will only build server requirements"
			BUILD_CLIENT=0
			BUILD_SERVER=1
			FEATURE_RENDERER2=0
			FEATURE_RENDERER_GLES=0
			RENDERER_DYNAMIC=0

			FEATURE_CURL=0
			FEATURE_OGG_VORBIS=0
			FEATURE_THEORA=0
			FEATURE_OPENAL=0
			FEATURE_FREETYPE=0
			FEATURE_PNG=0

			BUNDLED_SDL=0
			BUNDLED_ZLIB=1
			BUNDLED_MINIZIP=1
			BUNDLED_JPEG=0
			BUNDLED_OGG_VORBIS=0
			BUNDLED_THEORA=0
			BUNDLED_GLEW=0
			BUNDLED_FREETYPE=0
			BUNDLED_PNG=0
			BUNDLED_CURL=0
			BUNDLED_OPENAL=0
		else
			# drop the script commands from the result
			for index in ${!easy_keys[*]}
			do
				#printf "%4d: %s\n" $index ${easy_keys[$index]}
				if [ "$easy_count" = "$index" ] && [ ! "$var" = "${easy_keys[$index]}" ]; then
					if [ -z "$PARSE_CMD" ]; then
						PARSE_CMD=$var
					else
						PARSE_CMD="$PARSE_CMD $var"
					fi
				elif [[ "$var" = "${easy_keys[$index]}" ]]; then
					break
				fi
			done
		fi
	done
}

generate_configuration() {
	# Generator wich to use with CMake on the generate step
	MAKEFILE_GENERATOR=${MAKEFILE_GENERATOR:-Unix Makefiles}

	#cmake variables
	RELEASE_TYPE=${RELEASE_TYPE:-Release}
	CROSS_COMPILE32=${CROSS_COMPILE32:-1}
	ARM=${ARM:-0}
	BUILD_SERVER=${BUILD_SERVER:-1}
	BUILD_CLIENT=${BUILD_CLIENT:-1}
	BUILD_MOD=${BUILD_MOD:-1}
	BUILD_MOD_PK3=${BUILD_MOD_PK3:-1}
	BUILD_PAK_PK3=${BUILD_PAK_PK3:-1}
	ZIP_ONLY=${ZIP_ONLY:-0}
	BUNDLED_LIBS=${BUNDLED_LIBS:-1}
	BUNDLED_SDL=${BUNDLED_SDL:-1}
	BUNDLED_ZLIB=${BUNDLED_ZLIB:-1}
	BUNDLED_MINIZIP=${BUNDLED_MINIZIP:-1}
	BUNDLED_JPEG=${BUNDLED_JPEG:-1}
	BUNDLED_LUA=${BUNDLED_LUA:-1}
	BUNDLED_OGG_VORBIS=${BUNDLED_OGG_VORBIS:-1}
	BUNDLED_THEORA=${BUNDLED_THEORA:-1}
	BUNDLED_GLEW=${BUNDLED_GLEW:-1}
	BUNDLED_FREETYPE=${BUNDLED_FREETYPE:-1}
	BUNDLED_PNG=${BUNDLED_PNG:-1}
	BUNDLED_SQLITE3=${BUNDLED_SQLITE3:-1}

	if [ "${PLATFORMSYS}" != "Mac OS X" ] && [ "${PLATFORMSYS}" != "macOS" ]; then
		BUNDLED_CURL=${BUNDLED_CURL:-1}
		BUNDLED_OPENSSL=${BUNDLED_OPENSSL:-1}
		BUNDLED_OPENAL=${BUNDLED_OPENAL:-1}
	else
		BUNDLED_CURL=${BUNDLED_CURL:-0}
		BUNDLED_OPENSSL=${BUNDLED_OPENSSL:-0}
		BUNDLED_OPENAL=${BUNDLED_OPENAL:-0}
		CMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-10.12}
	fi

	FEATURE_RENDERER2=${FEATURE_RENDERER2:-0}
	FEATURE_RENDERER_GLES=${FEATURE_RENDERER_GLES:-0}
	RENDERER_DYNAMIC=${RENDERER_DYNAMIC:-1}

	FEATURE_CURL=${FEATURE_CURL:-1}
	FEATURE_SSL=${FEATURE_SSL:-1}
	FEATURE_OGG_VORBIS=${FEATURE_OGG_VORBIS:-1}
	FEATURE_THEORA=${FEATURE_THEORA:-1}
	FEATURE_OPENAL=${FEATURE_OPENAL:-1}
	FEATURE_FREETYPE=${FEATURE_FREETYPE:-1}
	FEATURE_PNG=${FEATURE_PNG:-1}
	FEATURE_GETTEXT=${FEATURE_GETTEXT:-1}
	FEATURE_DBMS=${FEATURE_DBMS:-1}
	FEATURE_LUA=${FEATURE_LUA:-1}
	FEATURE_MULTIVIEW=${FEATURE_MULTIVIEW:-1}
	FEATURE_EDV=${FEATURE_EDV:-1}
	FEATURE_ANTICHEAT=${FEATURE_ANTICHEAT:-1}
	FEATURE_RATING=${FEATURE_RATING:-1}
	FEATURE_PRESTIGE=${FEATURE_PRESTIGE:-1}
	FEATURE_AUTOUPDATE=${FEATURE_AUTOUPDATE:-1}
	FEATURE_LUASQL=${FEATURE_LUASQL:-1}
	INSTALL_EXTRA=${INSTALL_EXTRA:-1}
	INSTALL_GEOIP=${INSTALL_GEOIP:-1}
	INSTALL_WOLFADMIN=${INSTALL_WOLFADMIN:-1}

	if [ "${PLATFORMSYS}" != "Mac OS X" ] && [ "${PLATFORMSYS}" != "macOS" ]; then
		FEATURE_OMNIBOT=${FEATURE_OMNIBOT:-1}
		INSTALL_OMNIBOT=${INSTALL_OMNIBOT:-1}
	else
		# No Omnibot Support for OSX, so dismiss it
		FEATURE_OMNIBOT=0
		INSTALL_OMNIBOT=0
	fi

	einfo "Configuring ET: Legacy..."
	_CFGSTRING="
		-DCMAKE_BUILD_TYPE=${RELEASE_TYPE}
		-DCROSS_COMPILE32=${CROSS_COMPILE32}
		-DZIP_ONLY=${ZIP_ONLY}
		-DARM=${ARM}
		-DBUILD_SERVER=${BUILD_SERVER}
		-DBUILD_CLIENT=${BUILD_CLIENT}
		-DBUILD_MOD=${BUILD_MOD}
		-DBUILD_MOD_PK3=${BUILD_MOD_PK3}
		-DBUNDLED_LIBS=${BUNDLED_LIBS}
		-DBUNDLED_SDL=${BUNDLED_SDL}
		-DBUNDLED_ZLIB=${BUNDLED_ZLIB}
		-DBUNDLED_MINIZIP=${BUNDLED_MINIZIP}
		-DBUNDLED_JPEG=${BUNDLED_JPEG}
		-DBUNDLED_CURL=${BUNDLED_CURL}
		-DBUNDLED_WOLFSSL=${BUNDLED_WOLFSSL}
		-DBUNDLED_OPENSSL=${BUNDLED_OPENSSL}
		-DBUNDLED_LUA=${BUNDLED_LUA}
		-DBUNDLED_OGG_VORBIS=${BUNDLED_OGG_VORBIS}
		-DBUNDLED_THEORA=${BUNDLED_THEORA}
		-DBUNDLED_OPENAL=${BUNDLED_OPENAL}
		-DBUNDLED_GLEW=${BUNDLED_GLEW}
		-DBUNDLED_FREETYPE=${BUNDLED_FREETYPE}
		-DBUNDLED_PNG=${BUNDLED_PNG}
		-DBUNDLED_SQLITE3=${BUNDLED_SQLITE3}
		-DFEATURE_CURL=${FEATURE_CURL}
		-DFEATURE_SSL=${FEATURE_SSL}
		-DFEATURE_OGG_VORBIS=${FEATURE_OGG_VORBIS}
		-DFEATURE_THEORA=${FEATURE_THEORA}
		-DFEATURE_OPENAL=${FEATURE_OPENAL}
		-DFEATURE_FREETYPE=${FEATURE_FREETYPE}
		-DFEATURE_PNG=${FEATURE_PNG}
		-DFEATURE_LUA=${FEATURE_LUA}
		-DFEATURE_MULTIVIEW=${FEATURE_MULTIVIEW}
		-DFEATURE_EDV=${FEATURE_EDV}
		-DFEATURE_ANTICHEAT=${FEATURE_ANTICHEAT}
		-DFEATURE_GETTEXT=${FEATURE_GETTEXT}
		-DFEATURE_DBMS=${FEATURE_DBMS}
		-DFEATURE_RATING=${FEATURE_RATING}
		-DFEATURE_PRESTIGE=${FEATURE_PRESTIGE}
		-DFEATURE_AUTOUPDATE=${FEATURE_AUTOUPDATE}
		-DFEATURE_RENDERER2=${FEATURE_RENDERER2}
		-DFEATURE_RENDERER_GLES=${FEATURE_RENDERER_GLES}
		-DRENDERER_DYNAMIC=${RENDERER_DYNAMIC}
		-DFEATURE_LUASQL=${FEATURE_LUASQL}
		-DFEATURE_OMNIBOT=${FEATURE_OMNIBOT}
		-DINSTALL_EXTRA=${INSTALL_EXTRA}
		-DINSTALL_OMNIBOT=${INSTALL_OMNIBOT}
		-DINSTALL_GEOIP=${INSTALL_GEOIP}
		-DINSTALL_WOLFADMIN=${INSTALL_WOLFADMIN}
	"

	if [ "${DEV}" != 1 ]; then
	if [ "${PLATFORMSYS}" == "Mac OS X" ] || [ "${PLATFORMSYS}" == "macOS" ]; then
		PREFIX=${INSTALL_PREFIX}
		_CFGSTRING="${_CFGSTRING}
		-DXCODE_SDK_PATH=${XCODE_SDK_PATH}
		-DCMAKE_INSTALL_PREFIX=${PREFIX}
		-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
		-DINSTALL_DEFAULT_MODDIR=./
		-DINSTALL_DEFAULT_BINDIR=./
		-DINSTALL_DEFAULT_BASEDIR=./
		"
	else
		PREFIX=${INSTALL_PREFIX}
		_CFGSTRING="${_CFGSTRING}
		-DCMAKE_INSTALL_PREFIX=${PREFIX}

		-DINSTALL_DEFAULT_MODDIR=.
		-DINSTALL_DEFAULT_BINDIR=.
		-DINSTALL_DEFAULT_BASEDIR=.
		"
	fi
	fi

	echo -e "$boldyellowusing: $boldwhite${_CFGSTRING}$reset"
}

# Check if the bundled libs repo has been loaded
handle_bundled_libs() {
	if [[ ! -e "${_SRC}/libs/CMakeLists.txt" ]]; then
		einfo "Getting bundled libs..."
		git submodule init
		git submodule update
	fi
}

run_clean() {
	einfo "Clean..."
	if [ -d ${BUILDDIR} ]; then
		rm -rf ${BUILDDIR}
	fi
	CLEANLIBS=1
	if [[ -e "${_SRC}/libs/CMakeLists.txt" && ${CLEANLIBS} ]]; then
		# this doesn't work?
		if [ "${BUNDLED_SDL}" == 1 ]; then
			einfo "Cleaning SDL..."
			cd ${_SRC}/libs/sdl2;  make clean
		fi
		if [ "${BUNDLED_ZLIB}" == 1 ]; then
			einfo "Cleaning ZLib..."
			cd ${_SRC}/libs/zlib; make distclean
		fi
		if [ "${BUNDLED_MINIZIP}" == 1 ]; then
			einfo "Cleaning MiniZip..."
			cd ${_SRC}/libs/minizip; make clean
		fi
		if [ "${BUNDLED_JPEG}" == 1 ]; then
			einfo "Cleaning libjpeg-turbo..."
			cd ${_SRC}/libs/jpegturbo; make clean
		fi
		if [ "${BUNDLED_CURL}" == 1 ]; then
			einfo "Cleaning libcurl..."
			cd ${_SRC}/libs/curl/src; make clean
		fi
		if [ "${BUNDLED_LUA}" == 1 ]; then
			einfo "Cleaning Lua..."
			cd ${_SRC}/libs/lua/src; make clean
		fi
		if [ "${BUNDLED_OGG_VORBIS}" == 1 ]; then
			einfo "Cleaning libogg..."
			cd ${_SRC}/libs/ogg; make clean
			einfo "Cleaning libvorbis..."
			cd ${_SRC}/libs/vorbis; make clean
			# einfo "Cleaning libtheora..."
			# cd ${_SRC}/libs/theora; make clean
		fi
		if [ "${BUNDLED_OPENAL}" == 1 ]; then
			einfo "Cleaning openAL..."
			cd ${_SRC}/libs/openal; make clean
		fi
		if [ "${BUNDLED_PNG}" == 1 ]; then
			einfo "Cleaning libpng..."
			cd ${_SRC}/libs/libpng; make clean
		fi

		cd ${_SRC}/libs
		git clean -d -f
	fi
}

run_generate() {
	einfo "Generating makefiles: ${MAKEFILE_GENERATOR}..."
	mkdir -p ${BUILDDIR}
	cd ${BUILDDIR}
	cmake -G "${MAKEFILE_GENERATOR}" ${_CFGSTRING} ..
	check_exit
}

run_build() {
	run_generate
	einfo "Build..."
	make ${CMD_ARGS}
	cmake --build . --config $RELEASE_TYPE
	check_exit
}

set_osx_folder_icon_python() {
	# Needs to be the osx:s default python install!
	/usr/bin/python << END
import Cocoa
import sys
import os
import glob
import shutil
iconfile = "icon.png"
foldername = "ETLegacy"
if os.path.isdir(foldername):
	shutil.rmtree(foldername)
files = [f for f in glob.glob('./_CPack_Packages/Darwin/TGZ/etlegacy*') if os.path.isdir(f)]
if len(files) == 1 :
	packfolder = files[0]
	shutil.copytree(packfolder, foldername)
	print 'Copied the mod install folder'
	Cocoa.NSWorkspace.sharedWorkspace().setIcon_forFile_options_(Cocoa.NSImage.alloc().initWithContentsOfFile_(iconfile), foldername, 0) or sys.exit("Unable to set file icon")
	print 'The icon succesfully set'
END
}

set_osx_folder_icon_tooled() {
	# Use MacOS developer tools
	# Set the image as its own icon
	sips -i icon.png
	# Export the icon data
	DeRez -only icns icon.png > tmpicons.rscs
	# make sure we are staring with a clean slate
	rm -Rf ./ETLegacy
	# Copy the CPack generate folder here
	cp -R ./_CPack_Packages/Darwin/TGZ/etlegacy*/ ./ETLegacy
	# Append the image data into a special icon file inside the folder
	Rez -append tmpicons.rscs -o $'ETLegacy/Icon\r'
	# Set the icon for the folder
	SetFile -a C ETLegacy
	# Hide the icon file
	SetFile -a V $'ETLegacy/Icon\r'
}

create_ready_osx_dmg() {
	# Create the DMG json
	cat << END > etlegacy-dmg.json
{
	"title": "ET Legacy $SHORT_VERSION",
	"icon": "../misc/etl.icns",
	"background": "osx-dmg-background.jpg",
	"window": {
	  "size": {
		  "width": 640,
		  "height": 390
	  }
	},
	"contents": [
		{ "x": 456, "y": 250, "type": "link", "path": "/Applications" },
		{ "x": 192, "y": 250, "type": "file", "path": "ETLegacy" }
	]
}
END

	# using appdmg nodejs application to generate the actual DMG installer
	# https://github.com/LinusU/node-appdmg
	# npx --yes -p "appdmg@0.6.0" -c "appdmg etlegacy-dmg.json 'etlegacy-${ETLEGACY_VERSION}.dmg'"
	npx appdmg etlegacy-dmg.json "etlegacy-${ETLEGACY_VERSION}.dmg"
}

create_osx_dmg() {
	# Generate DMG
	app_exists APP_FOUND "gm"
	if [ $APP_FOUND == 0 ]; then
		echo "Missing GraphicsMagick skipping OSX installer creation"
		return
	fi

	app_exists APP_FOUND "node"
	if [ $APP_FOUND == 0 ]; then
		echo "Missing nodejs skipping OSX installer creation"
		return
	fi

	app_exists APP_FOUND "npx"
	if [ $APP_FOUND == 0 ]; then
		echo "Missing npx skipping OSX installer creation"
		return
	fi

	app_exists APP_FOUND "rsvg-convert"
	if [ $APP_FOUND == 0 ]; then
		echo "Missing rsvg-convert cannot create installer"
		exit 1
	fi

	echo "Generating OSX installer"
	SHORT_VERSION=`git describe --abbrev=0 --tags 2>/dev/null`

	# Generate the icon for the folder
	# using rsvg-convert
	# brew install librsvg
	rsvg-convert -h 256 ../misc/etl.svg > icon.png

	# Generate the DMG background
	# using the Graphics Magick
	# brew install graphicsmagick
	gm convert ../misc/osx-dmg-background.jpg -resize 640x360 -font ../misc/din1451alt.ttf -pointsize 20 -fill 'rgb(85,85,85)'  -draw "text 75,352 '${SHORT_VERSION}'" osx-dmg-background.jpg

	set_osx_folder_icon_tooled
	create_ready_osx_dmg
}

run_package() {
	einfo "Package..."
	cd ${BUILDDIR}
	# check_exit "make package"
	# calling cpack directly we are not checking the build output anymore
	check_exit "cpack"
	# TODO: detect if osx and generate a package and a dmg installer
	if [ "${PLATFORMSYS}" == "Mac OS X" ] || [ "${PLATFORMSYS}" == "macOS" ]; then
		create_osx_dmg
	fi
}

run_install() {
	einfo "Install..."
	cd ${BUILDDIR}
	check_exit "make install"
}

handle_download() {
	if [ ! -f $1 ]; then
		if [ -f /usr/bin/curl  ]; then
			curl -O "${ETLEGACY_MIRROR}$1"
		else
			wget "${ETLEGACY_MIRROR}$1"
		fi
	fi
}

run_download() {
	einfo "Downloading packages..."
	mkdir -p ${MODMAIN}
	cd ${MODMAIN}
	handle_download "pak0.pk3"
}

run_uncrustify() {
	einfo "Uncrustify..."
	cd ${SOURCEDIR}
	for FILE in $(find . -type f -not -name "unzip.c" -name "*.c" -or -name "*.cpp" -or -name "*.glsl" -not -name "g_etbot_interface.cpp" -or -name "*.h" -or \( -name "sha*" -prune \) -or \( -name "Omnibot" -prune \));
	do
		uncrustify -c ${_SRC}/uncrustify.cfg  --no-backup ${FILE}
	done
}

run_project() {
	einfo "Project..."
	if [ -d ${PROJECTDIR} ]; then
		rm -rf ${PROJECTDIR}
	fi
	mkdir -p ${PROJECTDIR}
	cd ${PROJECTDIR}
	if [ "${PLATFORMSYS}" == "Mac OS X" ] || [ "${PLATFORMSYS}" == "macOS" ]; then
		cmake -G 'Xcode' ${_CFGSTRING} ..
	else
		cmake ${_CFGSTRING} ..
	fi
}

run_release() {
	einfo "Doing a release build process..."
	run_clean
	run_build
	run_package
}

run_default() {
	einfo "Default build..."
	run_clean
	run_build
	run_package
	run_install
}

print_help() {
	ehead "ET: Legacy Easy Builder Help"
	ehead "==============================="
	ehead "clean - clean up the build"
	ehead "build - run the build process"
	ehead "generate - generate the build files"
	ehead "package - run the package process"
	ehead "install - install the game into the system"
	ehead "download - download assets"
	ehead "crust - run the uncrustify to the source"
	ehead "project - generate the project files for your platform"
	ehead "release - run the entire release process"
	ehead "help - print this help"
	echo
	einfo "Properties"
	ehead "-64, -32, -debug, -clang, -nodb -nor2, -nodynamic, -systemlib, -noextra, -noupdate, -mod, -server"
	ehead "--build=*, --prefix=*, --osx=*"
	echo
}

start_script() {
	setup_sensible_defaults

	parse_commandline $@

	#CMD_ARGS="${@:2}"
	#CMD_ARGS=$@
	CMD_ARGS=$PARSE_CMD

	print_startup

	handle_bundled_libs

	generate_configuration

	ARG_FOUND=0

	# Everything looks ok, try to run this shit!

	# Find and run the processes the user requested
	for var in "$@"
	do
		for index in ${!easy_keys[*]}
		do
			if [ "$var" = "${easy_keys[$index]}" ]; then
				eval "${easy_cmd[$index]}"
				ARG_FOUND=1
			fi
		done
	done

	if [ $ARG_FOUND -eq 1 ]; then
		return
	fi

	# No process was set run the default
	run_default
}

start_script $@

# Return to the original path
cd ${_SRC}
