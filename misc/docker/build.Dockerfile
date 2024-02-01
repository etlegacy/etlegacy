FROM centos:7
LABEL version="1.5"
LABEL maintainer="mail@etlegacy.com"
LABEL description="Linux build machine for the 32 and 64 bit linux releases"

# We will run the installations which will also install the very old system git
# so after the bulk install, we remove the system git and install an up to date client.
RUN cd /tmp && \
	echo "multilib_policy=all" >> /etc/yum.conf && \
	yum --assumeyes install https://repo.ius.io/ius-release-el7.rpm \
	https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm && \
	yum update --assumeyes --skip-broken && \
	yum groupinstall --assumeyes "Development tools" && \
	yum --assumeyes --exclude=git* install tar freeglut-devel gmake glibc glibc-devel libX11 \
	libX11-devel mesa-libGL mesa-libGL-devel alsa-lib-devel pulseaudio-libs-devel \
	curl-devel zlib-devel wget nasm which libXxf86vm-devel perl-IPC-Cmd \
	wayland-devel mesa-libEGL-devel mesa-libGLES-devel libxkbcommon-devel libXi-devel libXfixes-devel \
	libXScrnSaver-devel libXcursor-devel libXinerama-devel libXrandr-devel libXvmc-devel perl-Thread-Queue && \
	yum --assumeyes remove git && \
	yum --assumeyes install git236 && \
	yum clean all && \
	rm -rf /var/cache/yum && \
	rm -rf /var/tmp/yum-*

RUN	wget https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.gz && tar -xvzf m4-1.4.19.tar.gz && cd m4-1.4.19 && ./configure --prefix=/usr/local && make && make install && cd .. && \
	wget https://ftp.gnu.org/pub/gnu/libtool/libtool-2.4.7.tar.gz && tar -xvzf libtool-2.4.7.tar.gz && cd libtool-2.4.7 && ./configure --prefix=/usr/local && make && make install && cd .. && \
	wget https://ftp.gnu.org/gnu/autoconf/autoconf-2.71.tar.gz && tar -xvzf autoconf-2.71.tar.gz && cd autoconf-2.71 && ./configure --prefix=/usr/local && make && make install && cd .. && \
	wget https://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz && tar -xvzf automake-1.15.tar.gz && cd automake-1.15 && ./configure --prefix=/usr/local && make && make install && cd .. && \
	rm -Rf m4-1.4.19* libtool-2.4.7* autoconf-2.71* automake-1.15*

RUN mkdir -p /opt/cmake && wget --no-check-certificate --quiet -O - https://cmake.org/files/v3.28/cmake-3.28.2-linux-x86_64.tar.gz | tar --strip-components=1 -xz -C /opt/cmake
ENV PATH="/opt/cmake/bin:${PATH}"

# SDL2 now requires a newer wayland version >= 1.18 (we still install the older packages for the dependencies) so remove the pre-installed one from the system and build new ones
# compile 64 and 32 bit wayland
RUN rpm -e --nodeps --allmatches libwayland-client libwayland-cursor libwayland-egl libwayland-server wayland-devel && \
    yum --assumeyes install libffi-devel expat-devel libxml2-devel && rm -rf /var/cache/yum && rm -rf /var/tmp/yum-* && \
    wget --quiet -O - https://wayland.freedesktop.org/releases/wayland-1.18.0.tar.xz | tar -xJ && cd wayland-1.18.0 && \
    export PKG_CONFIG_PATH=/usr/lib64/pkgconfig && \
    ./configure --prefix=/usr --disable-static --disable-documentation --libdir=/usr/lib64 && make && make install && \
    make clean && export PKG_CONFIG_PATH=/usr/lib/pkgconfig && \
    ./configure --prefix=/usr --disable-static --disable-documentation --libdir=/usr/lib --host=i686-linux-gnu "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32" && make && make install && \
    cd .. && rm -Rf wayland-1.18.0 && unset PKG_CONFIG_PATH

RUN git clone --branch v1.11.1 --depth 1 https://github.com/ninja-build/ninja.git && cmake -B ninja/build -S ninja && cmake --build ninja/build && \
	cmake --install ninja/build && rm -Rf ninja

# RUN groupadd -g 2000 legacy && useradd -m -u 2001 -g legacy legacy && chmod -R 755 /opt/
# USER legacy

VOLUME /code
WORKDIR /code
