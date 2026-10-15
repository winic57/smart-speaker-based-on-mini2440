#ifndef CONF_H
#define CONF_H

#include <iostream>

//服务器配置，main.cpp 启动时从配置文件加载
struct server_conf
{
	char ip[64];        //监听 IP，0.0.0.0 表示所有网卡
	int  port;
};
extern server_conf g_sconf;

//读取配置文件，失败用默认值
void load_server_config(const char *path);

#endif
