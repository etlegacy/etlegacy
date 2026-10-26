# ET: Legacy build environment: AlmaLinux 10, GCC Toolset 15, CMake 4.4.3.
# Supports native x86_64 and -m32 i686 builds (subject to project dependencies).
FROM almalinux:10

LABEL version="2.0"
LABEL maintainer="mail@etlegacy.com"
LABEL description="Linux build machine for the 32 and 64 bit linux releases"

ARG CMAKE_VERSION=4.4.3

RUN dnf -y install dnf-plugins-core && \
    dnf config-manager --set-enabled crb && \
    dnf clean all

# AlmaLinux 10 keeps i686 packages in separate vault repositories.
# Restrict these repositories to i686 so native packages use the usual mirrors.
# Omit documentation because its generated files can conflict across architectures.
RUN for repo in BaseOS AppStream CRB; do \
      printf '%s\n' \
        "[multilib-${repo}]" \
        "name=AlmaLinux 10 - ${repo} - i686" \
        "baseurl=https://vault.almalinux.org/\$releasever/${repo}/i686/os/" \
        'enabled=1' \
        'gpgcheck=1' \
        'gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-AlmaLinux-10' \
        'includepkgs=*.i686' \
        > "/etc/yum.repos.d/multilib-${repo}.repo"; \
    done && \
    dnf -y --setopt=tsflags=nodocs install \
      gcc-toolset-15-gcc gcc-toolset-15-gcc-c++ gcc-toolset-15-binutils \
      glibc-devel.x86_64 glibc-devel.i686 \
      libgcc.x86_64 libgcc.i686 \
      libstdc++-devel.x86_64 libstdc++-devel.i686 \
      gcc-toolset-15-libstdc++-devel.x86_64 gcc-toolset-15-libstdc++-devel.i686 \
      make ninja-build nasm git curl tar gzip xz unzip zip \
      pkgconf-pkg-config python3 perl which file patch diffutils \
      autoconf automake libtool \
      zlib-ng-compat-devel.x86_64 zlib-ng-compat-devel.i686 \
      alsa-lib-devel.x86_64 alsa-lib-devel.i686 \
      pulseaudio-libs-devel.x86_64 pulseaudio-libs-devel.i686 \
      wayland-devel.x86_64 wayland-devel.i686 wayland-protocols-devel \
      libxkbcommon-devel.x86_64 libxkbcommon-devel.i686 \
      libX11-devel.x86_64 libX11-devel.i686 \
      libXext-devel.x86_64 libXext-devel.i686 \
      libXrandr-devel.x86_64 libXrandr-devel.i686 \
      libXcursor-devel.x86_64 libXcursor-devel.i686 \
      libXi-devel.x86_64 libXi-devel.i686 \
      libXinerama-devel.x86_64 libXinerama-devel.i686 \
      libXtst-devel.x86_64 libXtst-devel.i686 \
      systemd-devel.x86_64 systemd-devel.i686 \
      libusb1-devel.x86_64 libusb1-devel.i686 \
      mesa-libGL-devel.x86_64 mesa-libGL-devel.i686 \
      mesa-libGLU-devel.x86_64 mesa-libGLU-devel.i686 \
      mesa-libEGL-devel.x86_64 mesa-libEGL-devel.i686 \
      && dnf clean all

# Official Kitware prebuilt x86_64 CMake distribution.
RUN curl -fL --retry 3 \
      "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
      -o /tmp/cmake.tar.gz && \
    mkdir -p /opt/cmake && \
    tar -xzf /tmp/cmake.tar.gz --strip-components=1 -C /opt/cmake && \
    rm /tmp/cmake.tar.gz

# GCC Toolset is installed under /opt/rh, and must be explicitly selected.
ENV PATH="/opt/rh/gcc-toolset-15/root/usr/bin:/opt/cmake/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/rh/gcc-toolset-15/root/usr/lib64"

# Check C and C++ compilation and linking for both target architectures.
RUN printf 'int main(void){return 0;}\n' >/tmp/check.c && \
    printf '%s\n' '#include <iostream>' 'int main(){std::cout << "ok";}' >/tmp/check.cpp && \
    gcc -m64 /tmp/check.c -o /tmp/check64 && \
    gcc -m32 /tmp/check.c -o /tmp/check32 && \
    g++ -m64 /tmp/check.cpp -o /tmp/check-cxx64 && \
    g++ -m32 /tmp/check.cpp -o /tmp/check-cxx32 && \
    file /tmp/check64 /tmp/check32 /tmp/check-cxx64 /tmp/check-cxx32 && \
    rm -f /tmp/check.c /tmp/check.cpp /tmp/check64 /tmp/check32 /tmp/check-cxx64 /tmp/check-cxx32 && \
    cmake --version && gcc --version

VOLUME /code
WORKDIR /code
