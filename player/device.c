//用于操作硬件设备

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "main.h"
#include <sys/ioctl.h>
#include "device.h"

extern int g_buttonfd;
extern int g_ledfd;
extern int g_mixerfd;
extern int g_serialfd;
extern int g_maxfd;

//RTM rootfs 可能没有这些设备节点，打开失败只警告，继续跑
int InitDriver()
{
	//打开按键设备文件
	g_buttonfd = open("/dev/buttons", O_RDONLY);
	if (-1 == g_buttonfd)
	{
		printf("warning: open /dev/buttons failed, keys disabled\n");
	}

	//打开led设备文件
	g_ledfd = open("/dev/leds", O_WRONLY);
	if (-1 == g_ledfd)
	{
		printf("warning: open /dev/leds failed, led disabled\n");
	}
	else
	{
		//所有LED灭
		int i;
		for (i = 0; i < 4; i++)
		{
			ioctl(g_ledfd, 0, i);
		}
	}

	//打开mixer设备文件
	g_mixerfd = open("/dev/mixer", O_RDWR);
	if (-1 == g_mixerfd)
	{
		printf("warning: open /dev/mixer failed, volume control disabled\n");
	}
	else if (g_mixerfd > g_maxfd)
	{
		g_maxfd = g_mixerfd;
	}

	//打开语音模块串口（ASRPRO UART2 -> mini2440 串口一 ttySAC1）
	//失败只警告，不影响网络/按键播放
	g_serialfd = InitSerial("/dev/ttySAC1");
	if (-1 == g_serialfd)
	{
		printf("warning: open /dev/ttySAC1 failed, voice control disabled\n");
	}

	return SUCCESS;
}

//初始化串口：8N1, 115200, 原始模式（课程 039 做法）
//成功返回文件描述符，失败返回 -1
int InitSerial(const char *dev)
{
	int fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	if (-1 == fd)
	{
		perror("open serial");
		return -1;
	}

	//恢复为阻塞读：O_NDELAY 下 select 可读后再 read 才是准的
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags & ~O_NDELAY);

	struct termios attr;
	memset(&attr, 0, sizeof(attr));
	if (0 != tcgetattr(fd, &attr))
	{
		perror("tcgetattr");
		close(fd);
		return -1;
	}

	cfmakeraw(&attr);                      //原始模式：不做任何特殊字符处理
	cfsetispeed(&attr, B115200);
	cfsetospeed(&attr, B115200);
	attr.c_cflag |= (CLOCAL | CREAD);      //忽略 modem 控制线，允许接收
	attr.c_cflag &= ~CSIZE;
	attr.c_cflag |= CS8;                   //8 数据位
	attr.c_cflag &= ~PARENB;               //无校验
	attr.c_cflag &= ~CSTOPB;               //1 停止位
	attr.c_cc[VMIN] = 1;
	attr.c_cc[VTIME] = 0;

	if (0 != tcsetattr(fd, TCSANOW, &attr))  //立即生效
	{
		perror("tcsetattr");
		close(fd);
		return -1;
	}

	tcflush(fd, TCIOFLUSH);                //清掉缓冲区里的残留

	return fd;
}

void led_on(int which)
{
	if (-1 != g_ledfd)
	{
		ioctl(g_ledfd, 1, which);
	}
}

void led_off(int which)
{
	if (-1 != g_ledfd)
	{
		ioctl(g_ledfd, 0, which);
	}
}

int get_key_id()
{
	if (-1 == g_buttonfd)
	{
		return 0;
	}

	char buttons[6] = {'0', '0', '0', '0', '0', '0'};
	char cur_buttons[6] = {0};

	int ret = read(g_buttonfd, cur_buttons, sizeof(cur_buttons));
	if (-1 == ret)
	{
		perror("read");
	}

	int i;
	for (i = 0; i < 6; i++)
	{
		if (buttons[i] != cur_buttons[i])
		{
			return i + 1;
		}
	}
	return 0;
}
