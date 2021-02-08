# Use the build stage to load up the tar and unpack it
FROM debian:stable-slim AS builder
COPY etlegacy*.tar.gz /legacy/server/
RUN mkdir /legacy/homepath
RUN cd /legacy/server && cat *.tar.gz | tar zxvf - -i --strip-components=1 && rm *.tar.gz
RUN rm /legacy/server/etl && rm /legacy/server/etl_bot.sh && rm /legacy/server/*.so

FROM debian:stable-slim
RUN useradd -Ms /bin/bash legacy
COPY --from=builder --chown=legacy:legacy /legacy /legacy/
WORKDIR /legacy/server

# This can be used to mount a path for files to be written like logiles, or config files
VOLUME /legacy/homepath

# This can be used to mount a "readonly" volume for etmain pk3's and maps
VOLUME /legacy/server/etmain

EXPOSE 27960/UDP

USER legacy

ENTRYPOINT ["./etlded", "+set","fs_homepath", "/legacy/homepath", "+set", "g_protect", "1", "+exec", "etl_server.cfg"]
