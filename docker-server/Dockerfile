# syntax=docker/dockerfile:1
## 在镜像里从源码编译 player-server。
## 运行机是 aarch64（LPA3399Pro）时：docker build -t speaker-server:latest .
## 本机是 x86 要出 arm64 包：docker build --platform linux/arm64 -t speaker-server:latest .

FROM debian:buster AS build

RUN printf '%s\n' \
  'deb http://mirrors.aliyun.com/debian-archive/debian buster main' \
  'deb http://mirrors.aliyun.com/debian-archive/debian-security buster/updates main' \
  > /etc/apt/sources.list && \
  apt-get update && \
  apt-get install -y --no-install-recommends g++ libevent-dev libjsoncpp-dev && \
  rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY main.cpp server.cpp player.cpp conf.cpp ./
COPY server.h player.h conf.h node.h ./
RUN g++ -O2 -o /src/player-server main.cpp server.cpp player.cpp conf.cpp -levent -ljsoncpp

FROM debian:buster

RUN printf '%s\n' \
  'deb http://mirrors.aliyun.com/debian-archive/debian buster main' \
  'deb http://mirrors.aliyun.com/debian-archive/debian-security buster/updates main' \
  > /etc/apt/sources.list && \
  apt-get update && \
  apt-get install -y --no-install-recommends libevent-2.1-6 libjsoncpp1 && \
  rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/player-server /app/player-server
COPY player-server.conf /etc/player-server.conf

EXPOSE 8000
CMD ["/app/player-server", "/etc/player-server.conf"]
