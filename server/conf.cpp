#include "conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

server_conf g_sconf;

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

void load_server_config(const char *path)
{
	//默认值
	strcpy(g_sconf.ip, "0.0.0.0");
	g_sconf.port = 8000;

	FILE *fp = fopen(path, "r");
	if (NULL == fp)
	{
		std::cout << "warning: " << path << " 不存在，使用默认配置 0.0.0.0:8000" << std::endl;
		return;
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

		if (!strcmp(key, "ip"))
		{
			strncpy(g_sconf.ip, val, sizeof(g_sconf.ip) - 1);
		}
		else if (!strcmp(key, "port"))
		{
			g_sconf.port = atoi(val);
		}
	}

	fclose(fp);
	std::cout << "配置: listen=" << g_sconf.ip << ":" << g_sconf.port << std::endl;
}
