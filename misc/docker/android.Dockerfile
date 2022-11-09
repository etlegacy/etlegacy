FROM thyrlian/android-sdk:latest
LABEL version="1.0"
LABEL maintainer="mail@etlegacy.com"
LABEL description="Linux build machine for the android releases"

# Upgrade the system to be the most up to date
# We will later decide which libs to install  
RUN apt update \
	apt upgrade -y \
	apt-get autopurge -y \
	apt-get clean
	
# Image comes with availability of cmake 3.22 to be installed with but steal it from main build
RUN mkdir -p /opt/cmake && wget --no-check-certificate --quiet -O - https://cmake.org/files/v3.24/cmake-3.24.2-linux-x86_64.tar.gz | tar --strip-components=1 -xz -C /opt/cmake
ENV PATH="/opt/cmake/bin:${PATH}"

VOLUME /code
WORKDIR /code