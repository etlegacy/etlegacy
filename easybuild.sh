#!/usr/bin/env bash
# encoding: utf-8

# Made by the ET Legacy team!
# script checks for needed applications
# and builds ET Legacy

# Mandatory variables
_SRC=`pwd`
BUILDDIR="${_SRC}/build"
SOURCEDIR="${_SRC}/src"
PROJECTDIR="${_SRC}/project"
LEGACYETMAIN="${HOME}/.etlegacy/etmain"
LEGACY_MIRROR="http://mirror.etlegacy.com/etmain/"
LEGACY_VERSION=`git describe 2>/dev/null`

# Command that can be run
# first array has the cmd names which can be given
# second array holds the functions which match the cmd names
easy_keys=(clean build package install download crust release project help)
easy_cmd=(run_clean run_build run_package run_install run_download run_uncrustify run_release run_project print_help)
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

einfo() {
	echo -e "\n\033[1;32m~~>\033[0m \033[1;37m${1}\033[0m"
}

ehead() {
	echo -e "\033[1;36m * \033[1;37m${1}\033[0m"
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

check_compiler() {
	if [ -z "$CC" ] && [ -z "$CXX" ]; then
		app_exists GCCFOUND "gcc"
		app_exists GPLUSFOUND "g++"
		app_exists CLANGFOUND "clang"
		app_exists CLANGPLUSFOUND "clang++"
		if [ $GCCFOUND == 1 ] && [ $GPLUSFOUND == 1 ]; then
			export CC=gcc
			export CXX=g++
		elif [ $CLANGFOUND == 1 ] && [ $CLANGPLUSFOUND == 1 ]; then
			export CC=clang
			export CXX=clang++
		else
			einfo "Missing compiler. Exiting."
			exit 1
		fi
	fi
}

print_startup() {
	echo
	ehead "ET Legacy Easy Builder"
	ehead "==============================="
	ehead "This script will check for binaries needed to compile ET Legacy"
	ehead "Then it will build ET Legacy into ${BUILDDIR}/ directory"
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
	checkapp nasm
	echo

	check_compiler

	einfo "Using compilers:"

	echo
	echo "  CC  = ${CC}"
	echo "  CXX = ${CXX}"
}

parse_commandline() {
	for var in "$@"
	do
		if [ "$var" = "-64" ]; then
			einfo "Will disable crosscompile"
			CROSS_COMPILE32=0
		elif [ "$var" = "-clang" ]; then
			einfo "Will use clang"
			export CC=clang
			export CXX=clang++
		elif [ "$var" = "-debug" ]; then
			einfo "Will enable debug build"
			RELEASE_TYPE="Debug"
		elif [ "$var" = "-r2" ]; then
			einfo "Will enable renderer2"
			FEATURE_RENDERER2=1
		elif [ "$var" = "-dynamic" ]; then
			einfo "Will enable dynamic renderer build"
			RENDERER_DYNAMIC=1
		elif [ "$var" = "-jpeg-turbo" ]; then
			einfo "Will enable system jpeg turbo"
			BUNDLED_JPEG=0
		elif [ "$var" = "-systemlib" ]; then
			einfo "Will disable bundled libraries"
			BUNDLED_LIBS=0
			BUNDLED_SDL=0
			BUNDLED_JPEG=0
			BUNDLED_LUA=0
			BUNDLED_OGG=0
			BUNDLED_CURL=0
		elif [ "$var" = "-noob" ]; then
			einfo "Will disable omni-bot installation"
			INSTALL_OMNIBOT=0
		elif [ "$var" = "-noupdate" ]; then
			einfo "Will disable autoupdate"
			FEATURE_AUTOUPDATE=0
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
	#cmake variables
	RELEASE_TYPE=${RELEASE_TYPE:-Release}
	CROSS_COMPILE32=${CROSS_COMPILE32:-1}
	BUILD_SERVER=${BUILD_SERVER:-1}
	BUILD_CLIENT=${BUILD_CLIENT:-1}
	BUILD_MOD=${BUILD_MOD:-1}
	BUILD_MOD_PK3=${BUILD_MOD_PK3:-1}
	BUILD_PAK_PK3=${BUILD_PAK_PK3:-1}
	BUNDLED_LIBS=${BUNDLED_LIBS:-1}
	BUNDLED_SDL=${BUNDLED_SDL:-1}
	BUNDLED_JPEG=${BUNDLED_JPEG:-1}
	BUNDLED_LUA=${BUNDLED_LUA:-1}
	BUNDLED_OGG=${BUNDLED_OGG:-1}
	BUNDLED_GLEW=${BUNDLED_GLEW:-1}
	BUNDLED_FREETYPE=${BUNDLED_FREETYPE:-1}
	BUNDLED_JANSSON=${BUNDLED_JANSSON:-0}

	if [ "${PLATFORMSYS}" == "Mac OS X" ]; then
		BUNDLED_CURL=${BUNDLED_CURL:-0}
	else
		BUNDLED_CURL=${BUNDLED_CURL:-1}
	fi

	FEATURE_RENDERER2=${FEATURE_RENDERER2:-1}
	RENDERER_DYNAMIC=${RENDERER_DYNAMIC:-1}

	FEATURE_CURL=${FEATURE_CURL:-1}
	FEATURE_OGG=${FEATURE_OGG:-1}
	FEATURE_OPENAL=${FEATURE_OPENAL:-0}
	FEATURE_FREETYPE=${FEATURE_FREETYPE:-1}
	FEATURE_TRACKER=${FEATURE_TRACKER:-0}

	FEATURE_LUA=${FEATURE_LUA:-1}
	FEATURE_MULTIVIEW=${FEATURE_MULTIVIEW:-0}
	FEATURE_ANTICHEAT=${FEATURE_ANTICHEAT:-1}
	FEATURE_LIVEAUTH=${FEATURE_LIVEAUTH:-1}
	FEATURE_AUTOUPDATE=${FEATURE_AUTOUPDATE:-0}
	FEATURE_OMNIBOT=${FEATURE_OMNIBOT:-1}
	INSTALL_OMNIBOT=${INSTALL_OMNIBOT:-1}

	einfo "Configuring ET Legacy..."
	_CFGSTRING="
		-DCMAKE_BUILD_TYPE=${RELEASE_TYPE}
		-DCROSS_COMPILE32=${CROSS_COMPILE32}
		-DBUILD_SERVER=${BUILD_SERVER}
		-DBUILD_CLIENT=${BUILD_CLIENT}
		-DBUILD_MOD=${BUILD_MOD}
		-DBUILD_MOD_PK3=${BUILD_MOD_PK3}
		-DBUILD_PAK3_PK3=${BUILD_PAK_PK3}
		-DBUNDLED_LIBS=${BUNDLED_LIBS}
		-DBUNDLED_SDL=${BUNDLED_SDL}
		-DBUNDLED_JPEG=${BUNDLED_JPEG}
		-DBUNDLED_LUA=${BUNDLED_LUA}
		-DBUNDLED_CURL=${BUNDLED_CURL}
		-DBUNDLED_OGG_VORBIS=${BUNDLED_OGG}
		-DBUNDLED_GLEW=${BUNDLED_GLEW}
		-DBUNDLED_FREETYPE=${BUNDLED_FREETYPE}
		-DBUNDLED_JANSSON=${BUNDLED_JANSSON}
		-DFEATURE_CURL=${FEATURE_CURL}
		-DFEATURE_OGG_VORBIS=${FEATURE_OGG}
		-DFEATURE_OPENAL=${FEATURE_OPENAL}
		-DFEATURE_FREETYPE=${FEATURE_FREETYPE}
		-DFEATURE_TRACKER=${FEATURE_TRACKER}
		-DFEATURE_LUA=${FEATURE_LUA}
		-DFEATURE_MULTIVIEW=${FEATURE_MULTIVIEW}
		-DFEATURE_ANTICHEAT=${FEATURE_ANTICHEAT}
		-DFEATURE_GETTEXT=${FEATURE_GETTEXT}
		-DFEATURE_JANSSON=${FEATURE_JANSSON}
		-DFEATURE_LIVEAUTH=${FEATURE_LIVEAUTH}
		-DFEATURE_AUTOUPDATE=${FEATURE_AUTOUPDATE}
		-DFEATURE_RENDERER2=${FEATURE_RENDERER2}
		-DRENDERER_DYNAMIC=${RENDERER_DYNAMIC}
		-DFEATURE_OMNIBOT=${FEATURE_OMNIBOT}
		-DINSTALL_OMNIBOT=${INSTALL_OMNIBOT}
	"

	if [ "${DEV}" != 1 ]; then
	if [ "${PLATFORMSYS}" == "Mac OS X" ]; then
		PREFIX=${HOME}/etlegacy
		_CFGSTRING="${_CFGSTRING}
		-DCMAKE_INSTALL_PREFIX=${PREFIX}
		-DINSTALL_DEFAULT_MODDIR=./
		-DINSTALL_DEFAULT_BINDIR=./
		-DINSTALL_DEFAULT_BASEDIR=./
		"
	else
		PREFIX=${HOME}/etlegacy
		_CFGSTRING="${_CFGSTRING}
		-DCMAKE_INSTALL_PREFIX=${PREFIX}
		-DINSTALL_DEFAULT_MODDIR=.
		-DINSTALL_DEFAULT_BINDIR=.
		-DINSTALL_DEFAULT_BASEDIR=.
		"
	fi
	fi

	echo -e "\033[1;33musing: \033[1;37m${_CFGSTRING}\033[0m"
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
		if [ "${BUNDLED_SDL}" == 1 ]; then
			einfo "Cleaning SDL..."
			cd ${_SRC}/libs/sdl2;  make clean
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
		if [ "${BUNDLED_OGG}" == 1 ]; then
			einfo "Cleaning libogg..."
			cd ${_SRC}/libs/ogg; make clean
			einfo "Cleaning libvorbis..."
			cd ${_SRC}/libs/vorbis; make clean
		fi
		cd ${_SRC}/libs
		git clean -d -f
	fi
}

run_build() {
	einfo "Build..."
	mkdir -p ${BUILDDIR}
	cd ${BUILDDIR}
	cmake ${_CFGSTRING} ..
	check_exit
	make ${CMD_ARGS}
	check_exit
}

create_osx_dmg() {
	# Generate DMG
	app_exists APP_FOUND "dmgcanvas"
	if [ $APP_FOUND == 0 ]; then
		echo "Missing dmgcanvas skipping OSX installer creation"
		return
	fi

	app_exists APP_FOUND "rsvg-convert"
	if [ $APP_FOUND == 0 ]; then
		echo "Missing rsvg-convert cannot create installer"
		exit 1
	fi

	echo "Generating OSX installer"
	CANVAS_FILE="ETLegacy.dmgCanvas"

	# Generate the icon for the folder
	# using rsvg-convert
	# brew install librsvg
	rsvg-convert -h 256 ../misc/etl.svg > icon.png

	# Copy the canvas
	cp -rf ../misc/${CANVAS_FILE} ${CANVAS_FILE}

	# Needs to be the osx:s default python install!
	python << END
import Cocoa
import sys
import os
import glob
import shutil
iconfile = "icon.png"
foldername = "ET Legacy"
if os.path.isdir(foldername):
	shutil.rmtree(foldername)
files = [f for f in glob.glob('./_CPack_Packages/Darwin/TGZ/etlegacy*') if os.path.isdir(f)]
if len(files) == 1 :
	packfolder = files[0]
	shutil.copytree(packfolder, foldername)
	print 'Copied the legacy install folder'
	Cocoa.NSWorkspace.sharedWorkspace().setIcon_forFile_options_(Cocoa.NSImage.alloc().initWithContentsOfFile_(iconfile), foldername, 0) or sys.exit("Unable to set file icon")
	print 'The icon succesfully set'
END
	# We will be generating the dmg with the DMG Canvas app
	dmgcanvas ${CANVAS_FILE} "ETLegacy-${LEGACY_VERSION}.dmg" -v "ET Legacy ${LEGACY_VERSION}" -volume "ET Legacy ${LEGACY_VERSION}"
}

run_package() {
	einfo "Package..."
	cd ${BUILDDIR}
	# check_exit "make package"
	# calling cpack directly we are not checking the build output anymore
	check_exit "cpack"
	# TODO: detect if osx and generate a package and a dmg installer
	if [ "${PLATFORMSYS}" == "Mac OS X" ]; then
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
			curl -O "${LEGACY_MIRROR}$1"
		else
			wget "${LEGACY_MIRROR}$1"
		fi
	fi
}

run_download() {
	einfo "Downloading packages..."
	mkdir -p ${LEGACYETMAIN}
	cd ${LEGACYETMAIN}
	handle_download "pak0.pk3"
	handle_download "pak1.pk3"
	handle_download "pak2.pk3"
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
	mkdir -p ${BUILDDIR}
	cd ${PROJECTDIR}
	if [ "${PLATFORMSYS}" == "Mac OS X" ]; then
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
	ehead "ET Legacy Easy Builder Help"
	ehead "==============================="
	ehead "clean - clean up the build"
	ehead "build - run the build process"
	ehead "package - run the package process"
	ehead "install - install the game into the system"
	ehead "download - download assets"
	ehead "crust - run the uncrustify to the source"
	ehead "project - generate the project files for your platform"
	ehead "release - run the entire release process"
	ehead "help - print this help"
	echo
	einfo "Properties"
	ehead "-64, -debug, -clang, -r2, -dynamic, -systemlib, -noob, --noupdate"
	echo
}

start_script() {
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
