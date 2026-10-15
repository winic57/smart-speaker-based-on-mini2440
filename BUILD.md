# 编译和部署

这套代码实际分成三块：板上的 player、虚拟机里的 player-server、3399 上的 Docker 版 speaker-server。

## 板上 player（ARM，交叉编译）

在 RedHat 5 虚拟机里执行，不要在 Windows 上编。

- 编译器：`/usr/local/arm-4.4.3/bin/arm-linux-gcc`（4.4.3，链接到 arm-none-linux-gnueabi-gcc）
- 头文件和库：`/work/sysroot/include`、`/work/sysroot/lib`
- 依赖：json-c（旧接口 `json/json.h`）、pthread、`linux/soundcard.h`（来自 mini2440 的 sysroot，不是宿主机的）
- 源码目录按 `build.sh` 放在 `/work/smart-speaker-main`

```sh
sh /work/build.sh
```

只编 player 时：

```sh
cd /work/smart-speaker-main/player
/usr/local/arm-4.4.3/bin/arm-linux-gcc -o /work/out/asr-player \
  main.c device.c link.c player.c select.c socket.c config.c \
  -ljson-c -lpthread -I/work/sysroot/include -L/work/sysroot/lib
```

产物是 ELF ARM、给 Linux 2.6.32 用的动态链接程序。装到板子：

- 程序：`/usr/bin/asr-player`
- 配置：`/etc/speaker.conf`（样例是 `player/speaker.conf`）
- 开机启动必须脱离登录终端，否则 madplay 一写终端，整组进程会被壳停住，有命令也没有声音：

```sh
setsid /usr/bin/asr-player /etc/speaker.conf >/tmp/player.log 2>&1 </dev/null &
```

音乐在 SD 卡 `/sdcard/第1季` … `/sdcard/第12季`，文件名以 `.mp3` 结尾。断点写在 `/sdcard/peppa.pos`。语音模块接 `/dev/ttySAC1`，115200 8N1。

`player/speaker.conf` 里现在的默认服务器是 `192.168.50.129:8000`。换服务器只改这个文件，然后用上面的 setsid 命令重新启动。

## 虚拟机里的 player-server

同一台 RedHat 5 上，用本机 g++，不交叉编译：

```sh
cd /work/smart-speaker-main/server
g++ main.cpp server.cpp player.cpp conf.cpp -o /work/out/player-server -levent -ljsoncpp
```

依赖：libevent、jsoncpp。配置样例是 `server/player-server.conf`，默认读 `/etc/player-server.conf`，监听 `0.0.0.0:8000`。

这台虚拟机的防火墙如果走 `RH-Firewall-1-INPUT`，要放行 TCP 8000，否则板子连上来会被拒绝。

## Docker 版 speaker-server（当前默认服务器）

源码在 `docker-server/`，和 `server/` 是同一套信令程序。在 aarch64 主机上：

```sh
cd docker-server
docker build -t speaker-server:latest .
docker rm -f speaker-server
docker run -d --name speaker-server --restart unless-stopped -p 8000:8000 speaker-server:latest
```

镜像基于 Debian buster，编译依赖 `g++`、`libevent-dev`、`libjsoncpp-dev`，运行只要 `libevent-2.1-6` 和 `libjsoncpp1`。不要把编好的 `player-server` 二进制提交进仓库，镜像构建时自己编译。

x86 机器要给 3399 出包时加上 `--platform linux/arm64`。

## 不要提交的东西

PAT、交叉编译产物、`app/build/`、Qt 的 dll 和 moc 输出都不进仓库。
