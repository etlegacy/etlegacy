# ET: Legacy build environment: AlmaLinux 8, GCC Toolset 15, CMake 4.4.3.
# Native aarch64 (arm64) build machine; built and run on arm64 hosts.
FROM almalinux:8

LABEL version="2.0"
LABEL maintainer="mail@etlegacy.com"
LABEL description="Linux build machine for the arm64 (aarch64) linux releases"

ARG CMAKE_VERSION=4.4.3

RUN dnf -y install dnf-plugins-core && \
    dnf config-manager --set-enabled powertools && \
    dnf -y install \
      gcc-toolset-15-gcc gcc-toolset-15-gcc-c++ gcc-toolset-15-binutils \
      glibc-devel libgcc libstdc++-devel gcc-toolset-15-libstdc++-devel \
      make nasm git curl tar gzip xz unzip zip \
      pkgconf-pkg-config python3 perl which file patch diffutils \
      autoconf automake libtool \
      zlib-devel alsa-lib-devel pulseaudio-libs-devel \
      wayland-devel wayland-protocols-devel libxkbcommon-devel \
      libX11-devel libXext-devel libXrandr-devel libXcursor-devel \
      libXi-devel libXinerama-devel libXScrnSaver-devel libXtst-devel \
      systemd-devel libusbx-devel \
      mesa-libGL-devel mesa-libGLU-devel mesa-libEGL-devel \
      && dnf clean all

# Official Kitware prebuilt aarch64 CMake distribution.
RUN curl -fL --retry 3 \
      "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-aarch64.tar.gz" \
      -o /tmp/cmake.tar.gz && \
    mkdir -p /opt/cmake && \
    tar -xzf /tmp/cmake.tar.gz --strip-components=1 -C /opt/cmake && \
    rm /tmp/cmake.tar.gz

# GCC Toolset is installed under /opt/rh, and must be explicitly selected.
ENV PATH="/opt/rh/gcc-toolset-15/root/usr/bin:/opt/cmake/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/rh/gcc-toolset-15/root/usr/lib64"

# The ARM jobs are the only users of easybuild's -ninja; keep ninja 1.11.1
# known-good instead of EL8's older ninja-build package.
RUN git clone --branch v1.11.1 --depth 1 https://github.com/ninja-build/ninja.git && \
    cmake -B ninja/build -S ninja && \
    cmake --build ninja/build && \
    cmake --install ninja/build && rm -Rf ninja

# Smoke-test compilation and linking, not just compiler detection.
RUN printf 'int main(void){return 0;}\n' >/tmp/check.c && \
    gcc /tmp/check.c -o /tmp/check && \
    file /tmp/check && \
    rm -f /tmp/check.c /tmp/check && \
    cmake --version && gcc --version

VOLUME /code
WORKDIR /code
