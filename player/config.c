//配置文件解析：key value 行式格式，# 开头为注释

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

speaker_conf_t g_conf;

void default_config()
{
	strcpy(g_conf.server_ip, "192.168.50.129");
	g_conf.server_port = 8000;
	strcpy(g_conf.device_id, "001");
	strcpy(g_conf.music_path, "/sdcard/");
	strcpy(g_conf.madplay_path, "/bin/madplay");
	g_conf.keepalive = 5;
g_conf.play_minutes = 30;
}

//去掉首尾空白
static void trim(char *s)
{
	char *p = s;
	while (*p == ' ' || *p == '\t')
	{
		p++;
	}
	if (p != s)
	{
		memmove(s, p, strlen(p) + 1);
	}
	p = s + strlen(s);
	while (p > s && (p[-1] == ' ' || p[-1] == '\t' || p[-1] == '\r' || p[-1] == '\n'))
	{
		p--;
	}
	*p = '\0';
}

int load_config(const char *path)
{
	default_config();

	FILE *fp = fopen(path, "r");
	if (NULL == fp)
	{
		return FAILURE;
	}

	char line[256];
	while (fgets(line, sizeof(line), fp))
	{
		trim(line);
		if (line[0] == '#' || line[0] == '\0')
		{
			continue;
		}

		char *eq = strchr(line, '=');
		if (NULL == eq)
		{
			continue;
		}
		*eq = '\0';
		char *key = line;
		char *val = eq + 1;
		trim(key);
		trim(val);

		if (!strcmp(key, "server_ip"))
		{
			strncpy(g_conf.server_ip, val, sizeof(g_conf.server_ip) - 1);
		}
		else if (!strcmp(key, "server_port"))
		{
			g_conf.server_port = atoi(val);
		}
		else if (!strcmp(key, "device_id"))
		{
			strncpy(g_conf.device_id, val, sizeof(g_conf.device_id) - 1);
		}
		else if (!strcmp(key, "music_path"))
		{
			strncpy(g_conf.music_path, val, sizeof(g_conf.music_path) - 1);
		}
		else if (!strcmp(key, "madplay_path"))
		{
			strncpy(g_conf.madplay_path, val, sizeof(g_conf.madplay_path) - 1);
		}
		else if (!strcmp(key, "keepalive"))
		{
			g_conf.keepalive = atoi(val);
		}
		else if (!strcmp(key, "play_minutes"))
		{
			g_conf.play_minutes = atoi(val);
		}
	}

	fclose(fp);
	return SUCCESS;
}
