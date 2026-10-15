#!/bin/sh
# 智能音箱项目一键编译脚本（RHEL5 虚拟机内执行）
# 用法: sh /work/build.sh

set -e

ARM_GCC=/usr/local/arm-4.4.3/bin/arm-linux-gcc
WORK=/work/smart-speaker-main
OUT=/work/out

mkdir -p $OUT

echo "=== 1. 交叉编译 player (板上) ==="
cd $WORK/player
$ARM_GCC -o $OUT/player main.c device.c link.c player.c select.c socket.c config.c \
  -ljson-c -lpthread -I/work/sysroot/include -L/work/sysroot/lib
echo "player OK"

echo "=== 2. 编译 server (VM 本机) ==="
cd $WORK/server
g++ main.cpp server.cpp player.cpp conf.cpp -o $OUT/player-server -levent -ljsoncpp
echo "player-server OK"

echo "=== 全部完成，产物在 $OUT ==="
ls -la $OUT
