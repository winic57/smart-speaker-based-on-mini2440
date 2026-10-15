#ifndef CONFIG_H
#define CONFIG_H

#define SUCCESS     10000
#define FAILURE     10001

typedef struct speaker_conf
{
	char server_ip[64];
	int  server_port;
	char device_id[16];
	char music_path[128];
	char madplay_path[128];
	int  keepalive;
	int  play_minutes;
} speaker_conf_t;

extern speaker_conf_t g_conf;

int load_config(const char *path);
void default_config();

#endif
