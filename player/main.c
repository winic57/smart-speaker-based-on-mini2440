#include <stdio.h>
#include "main.h"
#include "device.h"
#include <stdlib.h>
#include "socket.h"
#include "link.h"
#include "player.h"
#include "config.h"
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/select.h>

int g_buttonfd = 3;
int g_ledfd;
int g_mixerfd;
int g_serialfd = -1;
int g_sockfd = 3;
struct Node *head;
extern int shmid;
extern void *g_addr;
fd_set readfd, tmpfd;
int g_maxfd;


void m_select();

void handler(int sig)
{
	shm s;
	memset(&s, 0, sizeof(s));
	memcpy(&s, g_addr, sizeof(s));
	kill(s.child_pid, SIGKILL);
	kill(s.grand_pid, SIGKILL);

	shmdt(g_addr);
	shmctl(shmid, IPC_RMID, NULL);

	close(g_sockfd);

	exit(0);
}

int main(int argc, char *argv[])
{
	int ret;
	ret = InitDriver();     //打开设备文件（失败只警告，不退出）
	if (FAILURE == ret)
	{
		printf("初始化设备文件失败\n");
		exit(1);
	}

	//加载配置文件：默认 /etc/speaker.conf，可用第一个参数指定
	const char *conf_path = (argc > 1) ? argv[1] : "/etc/speaker.conf";
	if (FAILURE == load_config(conf_path))
	{
		printf("warning: %s 不存在，使用默认配置\n", conf_path);
	}
	printf("配置: server=%s:%d deviceid=%s music=%s madplay=%s keepalive=%d play_minutes=%d\n",
		g_conf.server_ip, g_conf.server_port, g_conf.device_id,
		g_conf.music_path, g_conf.madplay_path, g_conf.keepalive, g_conf.play_minutes);
	
	InitSelect();

	ret = InitSocket();      //初始化网络
	if (FAILURE == ret)
	{
		printf("初始化网络失败\n");
		//初始化失败，点亮 2 个LED灯
		led_on(0);
		led_on(1);
	}

	//初始化链表
	ret = InitLink();
	if (FAILURE == ret)
	{
		printf("链表初始化失败\n");
		exit(1);
	}

	//初始化共享内存
	ret = InitShm();
	if (FAILURE == ret)
	{
		printf("共享内存初始化失败\n");
		exit(1);
	}

	//读取音乐
	init_library();

	//捕获ctrl c信号
	signal(SIGINT, handler);

	m_select();

	return 0;
}
