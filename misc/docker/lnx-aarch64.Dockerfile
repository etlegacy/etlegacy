FROM debian:11.8-slim
LABEL version="1.0"
LABEL maintainer="mail@etlegacy.com"
LABEL description="Linux build machine for the arm64 (aarch64) linux releases"

RUN cd /tmp && \
    dpkg --add-architecture arm64 && \
	apt-get update && apt-get install -y \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu git make cmake autoconf libtool zip patch nasm g++ wget pkg-config \
    libx11-dev:arm64 libgl1-mesa-dev:arm64 libxext-dev:arm64 xserver-xorg-dev:arm64 libwayland-dev:arm64 \
    libasound2-dev:arm64 libpulse-dev:arm64 libxrandr-dev:arm64 libxcursor-dev:arm64 libxi-dev:arm64 \
    libxss-dev:arm64 libgl1-mesa-dev:arm64 libglu1-mesa-dev:arm64

RUN apt-get install -y libxcb-xinput-dev:arm64 libwayland-egl1:arm64 libwayland-egl1-mesa:arm64 libwayland-cursor0:arm64 libxkbcommon-dev:arm64 libexpat-dev:arm64 libxml2-dev:arm64 && \
    wget --quiet -O - https://wayland.freedesktop.org/releases/wayland-1.18.0.tar.xz | tar -xJ && cd wayland-1.18.0 && \
    export PKG_CONFIG=/usr/bin/pkg-config && export PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig && \
    ./configure --prefix=/usr --disable-static --disable-documentation --libdir=/usr/aarch64-linux-gnu/lib/ --bindir=/usr/aarch64-linux-gnu/bin/ --host=aarch64-linux-gnu --with-host-scanner && \
    make && make install && make clean && \
    cd .. && rm -Rf wayland-1.18.0 && unset PKG_CONFIG_PATH

RUN git clone --branch v1.11.1 --depth 1 https://github.com/ninja-build/ninja.git && \
    cmake -B ninja/build -S ninja && \
    cmake --build ninja/build && \
	cmake --install ninja/build && rm -Rf ninja

ENV PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig

VOLUME /code
WORKDIR /code
