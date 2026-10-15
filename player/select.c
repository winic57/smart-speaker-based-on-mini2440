#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "player.h"
//#include <json-c/json.h>
#include <json/json.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "socket.h"
#include <errno.h>

extern int g_buttonfd;
extern int g_sockfd;
extern int g_serialfd;
extern fd_set readfd;
extern fd_set tmpfd;
extern int g_maxfd;

void show()
{
	printf("1、开始播放\n");
	printf("2、结束播放\n");
	printf("3、暂停播放\n");
	printf("4、继续播放\n");
	printf("5、上一首\n");
	printf("6、下一首\n");
	printf("7、增加音量\n");
	printf("8、减小音量\n");
	printf("9、顺序播放\n");
	printf("a、随即播放\n");
	printf("b、单曲循环\n");
}

void parse_message(const char *m, char *c)
{
	struct json_object *obj = json_tokener_parse(m);

	struct json_object *json;
	//json_object_object_get_ex(obj, "cmd", &json);
	json = json_object_object_get(obj, "cmd");
	strcpy(c, json_object_get_string(json));
}

void InitSelect()
{
	FD_ZERO(&readfd);
	FD_ZERO(&tmpfd);
	if (g_buttonfd >= 0)
		FD_SET(g_buttonfd, &readfd);
	if (g_serialfd >= 0)
	{
		FD_SET(g_serialfd, &readfd);
		if (g_serialfd > g_maxfd)
			g_maxfd = g_serialfd;
	}
}

void serial_dispatch(unsigned char ch);

void m_select()
{
	//关掉 stdout 缓冲：player 日志是被重定向到文件的，
	//默认全缓冲会导致菜单/串口命令要攒满 4K 才落盘，实时看不到
	setvbuf(stdout, NULL, _IONBF, 0);

	show();

	fd_set tmpfd;
	int ret;
	char message[1024] = {0};
	struct timeval tv;

	while (1)
	{
		tmpfd = readfd;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = select(g_maxfd + 1, &tmpfd, NULL, NULL, &tv);
		if (-1 == ret && errno != EINTR)
		{
			perror("select");
			continue;
		}
		else if (-1 == ret && errno == EINTR)
		{
			continue;
		}
		else if (0 == ret)
		{
			player_tick();
			continue;
		}

		player_tick();

		if (FD_ISSET(g_sockfd, &tmpfd))           //TCP有数据可读
		{
			memset(message, 0, sizeof(message));
			ret = recv(g_sockfd, message, sizeof(message), 0);
			if (ret <= 0)
			{
				if (ret < 0)
					perror("recv");
				else
					printf("server closed\n");
				FD_CLR(g_sockfd, &readfd);
				g_link_down = 1;
				continue;
			}

			char cmd[64] = {0};
			parse_message(message, cmd);

			if (!strcmp(cmd, "start"))
			{
				socket_start_play();			
			}
			else if (!strcmp(cmd, "stop"))
			{
				socket_stop_play();
			}
			else if (!strcmp(cmd, "suspend"))
			{
				socket_suspend_play();
			}
			else if (!strcmp(cmd, "continue"))
			{
				socket_continue_play();
			}
			else if (!strcmp(cmd, "prior"))
			{
				socket_prior_play();
			}
			else if (!strcmp(cmd, "next"))
			{
				socket_next_play();
			}
			else if (!strcmp(cmd, "voice_up"))
			{
				socket_voice_up_play();
			}
			else if (!strcmp(cmd, "voice_down"))
			{
				socket_voice_down_play();
			}
			else if (!strcmp(cmd, "sequence"))
			{
				socket_mode_play(SEQUENCEMODE);
			}
			else if (!strcmp(cmd, "random"))
			{
				socket_mode_play(RANDOM);
			}
			else if (!strcmp(cmd, "circle"))
			{
				socket_mode_play(CIRCLE);
			}
			else if (!strcmp(cmd, "get"))     //获取状态
			{	
				socket_get_status();
			}
			else if (!strcmp(cmd, "music"))   //获取所有音乐
			{
				socket_get_music();								
			}
		}
		else if (g_buttonfd >= 0 && FD_ISSET(g_buttonfd, &tmpfd))    //按键有数据可读
		{
			int id = get_key_id();
			switch (id)
			{
				case 1:
					start_play();
					break;
				case 2:
					stop_play();
					break;
				case 3:
					suspend_play();
					break;
				case 4:
					continue_play();
					break;
				case 5:
					prior_play();
					break;
				case 6:
					next_play();
					break;
			}
		}
		else if (g_serialfd >= 0 && FD_ISSET(g_serialfd, &tmpfd))    //语音模块串口有数据
		{
			unsigned char ch = 0;
			int n = read(g_serialfd, &ch, 1);
			if (n == 1)
			{
				serial_dispatch(ch);
			}
		}
	}
}

/*
串口命令分派：字节值来自 ASRPRO 天问Block 配置的「串口2输出」
（当前固件实测表，见 Obsidian「天问Block改唤醒词与识别标识」笔记）
*/
void serial_dispatch(unsigned char ch)
{
	//串口命令走 stderr：stderr 默认无缓冲，且便于和 stdout 分开看
	fprintf(stderr, "串口收到命令: 0x%02x\n", ch);

	switch (ch)
	{
		case 0x00:                          //唤醒词「小度小度」/ 未定义，忽略
			break;
		case 0x01:  start_play();      break;   //我想听歌
		case 0x02:  stop_play();       break;   //停止播放
		case 0x03:  suspend_play();    break;   //暂停一下
		case 0x04:  continue_play();   break;   //继续播放
		case 0x05:  next_play();       break;   //下一首
		case 0x06:  prior_play();      break;   //上一首
		case 0x07:  voice_up();        break;   //声音大一点
		case 0x08:  voice_down();      break;   //声音小一点
		default:
		{
			int season = 0;
			if (ch == 0x09)
				season = 1;
			else if (ch >= 0x10 && ch <= 0x19)
				season = (ch - 0x10) + 2;
			else if (ch == 0x20)
				season = 12;
			if (season > 0)
				play_season(season);
			break;
		}
	}
}
