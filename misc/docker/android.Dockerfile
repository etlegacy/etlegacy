FROM thyrlian/android-sdk:latest
LABEL version="1.0"
LABEL maintainer="mail@etlegacy.com"
LABEL description="Linux build machine for the android releases"

# Upgrade the system to be the most up to date
# We will later decide which libs to install
RUN apt update && apt upgrade -y && apt install ninja-build rename patch -y && apt autopurge -y && apt clean

VOLUME /code
WORKDIR /code
