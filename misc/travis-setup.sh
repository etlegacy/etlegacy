#!/usr/bin/env bash
# encoding: utf-8

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	# install needed packages
	echo "Travis running linux build"
	echo 'deb http://archive.ubuntu.com/ubuntu trusty main' | sudo tee /etc/apt/sources.list.d/ubuntu-trusty.list
  sudo apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 40976EAF437D05B5
  sudo apt-get update -qq
  sudo apt-get install -q -t precise pkg-config nasm libgl1-mesa-dev libasound2-dev pulseaudio m4
  sudo apt-get install -q -t trusty automake
elif [[ "$OSTYPE" == "darwin"* ]]; then
	# we need the gnu-sed from brew for r2
	echo "Travis running apple build"
	export BUNDLED_CURL=0
	export BUNDLED_SDL=0
	export BUNDLED_JPEG=0
	brew update
	brew install gnu-sed
	#brew install nasm
	#brew outdated automake || brew upgrade automake
	#brew outdated autoconf || brew upgrade autoconf
	brew install sdl2
	brew install jpeg-turbo
else
	# unknown system :/
	echo "Build might fail now"
fi

