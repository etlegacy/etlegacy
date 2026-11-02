FROM thyrlian/android-sdk:latest
LABEL version="1.0"
LABEL maintainer="mail@etlegacy.com"
LABEL description="Linux build machine for the android releases"

# Upgrade the system to be the most up to date
# We will later decide which libs to install
RUN apt update && apt upgrade -y && apt install ninja-build rename patch -y && apt autopurge -y && apt clean

# Preinstall the SDK components the Gradle build needs. Otherwise the Android
# Gradle plugin downloads them on every CI run, and a failed or truncated
# download (e.g. "Error on ZipFile unknown archive" for the NDK) breaks the build.
# Keep these versions in sync with app/build.gradle (ndkVersion, cmake version
# and compileSdk).
RUN yes | sdkmanager --licenses >/dev/null && \
    sdkmanager "ndk;27.0.12077973" "cmake;3.22.1" "platforms;android-36"

VOLUME /code
WORKDIR /code
