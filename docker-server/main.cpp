#include "server.h"

int main(int argc, char *argv[])
{
	//加载配置：默认 /etc/player-server.conf，可用第一个参数指定
	const char *conf_path = (argc > 1) ? argv[1] : "/etc/player-server.conf";
	load_server_config(conf_path);

	PlayerServer ps(g_sconf.ip, g_sconf.port);

	return 0;
}
