FROM debian:stable-slim

RUN useradd -Ms /bin/bash legacy && mkdir /legacy && mkdir /legacy/server && mkdir /legacy/homepath
COPY etlegacy*.tar.gz /legacy/server/
RUN cd /legacy/server && cat *.tar.gz | tar zxvf - -i --strip-components=1 && rm *.tar.gz
RUN chown -R legacy:legacy /legacy
WORKDIR /legacy/server

# This can be used to mount a path for files to be written like logiles, or config files
VOLUME /legacy/homepath

# This can be used to mount a "readonly" volume for etmain pk3's and maps
VOLUME /legacy/server/etmain

EXPOSE 27960/UDP

USER legacy

ENTRYPOINT ["./etlded", "+set","fs_homepath", "/legacy/homepath", "+set", "g_protect", "1", "+exec", "etl_server.cfg"]
