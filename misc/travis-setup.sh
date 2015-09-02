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
	# do we need something for apple?
	echo "Travis running apple build"
else
	# unknown system :/
	echo "Build might fail now"
fi

