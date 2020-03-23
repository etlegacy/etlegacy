FROM centos:6.10
LABEL maintainer="mail@etlegacy.com"
LABEL description="Linux build machine for the 32 and 64 bit linux releases"

RUN cd /tmp && \
	echo "multilib_policy=all" >> /etc/yum.conf && \
	yum update --assumeyes --skip-broken && \
	yum groupinstall --assumeyes "Development tools" && \
	yum --assumeyes install git tar freeglut-devel gmake glibc glibc-devel \
	libX11 libX11-devel mesa-libGL mesa-libGL-devel alsa-lib-devel pulseaudio-libs-devel \
	curl-devel zlib-devel wget nasm which libXxf86vm-devel && \
	yum clean all && \
	rm -rf /var/cache/yum && \
	rm -rf /var/tmp/yum-*

RUN	wget http://ftp.gnu.org/gnu/m4/m4-1.4.18.tar.gz && tar -xvzf m4-1.4.18.tar.gz && cd m4-1.4.18 && ./configure --prefix=/usr/local && make && make install && cd .. && \
	wget http://www.nic.funet.fi/pub/gnu/ftp.gnu.org/pub/gnu/libtool/libtool-2.4.6.tar.gz && tar -xvzf libtool-2.4.6.tar.gz && cd libtool-2.4.6 && ./configure --prefix=/usr/local && make && make install && cd .. && \
	wget http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz && tar -xvzf autoconf-2.69.tar.gz && cd autoconf-2.69 && ./configure --prefix=/usr/local && make && make install && cd .. && \
	wget http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz && tar -xvzf automake-1.15.tar.gz && cd automake-1.15 && ./configure --prefix=/usr/local && make && make install && cd .. && \
	rm -Rf m4-1.4.18* libtool-2.4.6* autoconf-2.69* automake-1.15*

RUN mkdir -p /opt/cmake && wget --no-check-certificate --quiet -O - https://cmake.org/files/v3.15/cmake-3.15.4-Linux-x86_64.tar.gz | tar --strip-components=1 -xz -C /opt/cmake
ENV PATH="/opt/cmake/bin:${PATH}"

# RUN groupadd -g 2000 legacy && useradd -m -u 2001 -g legacy legacy && chmod -R 755 /opt/
# USER legacy

VOLUME /code
WORKDIR /code
