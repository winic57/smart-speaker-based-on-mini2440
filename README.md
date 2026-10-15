# smart-speaker-based-on-mini2440
基于mini2440开发板实现的智能音箱，通过对音箱、后台服务器以及 APP 三部分的软件设计，实现了服务器对音箱及APP数据的并发处理以及APP对音箱的远程控制

板上播放器按 SD 卡里的「第N季」文件夹播放，断点保存在 SD 卡。编译器路径、依赖和部署命令见 [BUILD.md](BUILD.md)。默认服务器写在 `player/speaker.conf`。

## 架构

[mini2440 智能音箱架构](.archify/architecture-smart-speaker-20261015-182521/smart-speaker.html)
