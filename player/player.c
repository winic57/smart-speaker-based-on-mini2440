#include <stdio.h>
#include "link.h"
#include "main.h"
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "player.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <time.h>

extern int g_mixerfd;
extern Node *head;
void *g_addr = NULL;
int shmid;

int g_start_flag = 0;
int g_suspend_flag = 0;

int iLeft = 20;
int iRight = 60;

#define MAX_SONGS 160

static char g_root[128];
static char g_season_path[256];
static char g_track_name[128];
static int g_season = 0;
static int g_resume_season = 1;
static int g_pending_offset = 0;
static int g_track_sec = 0;
static int g_session_sec = 0;
static int g_since_save = 0;
static int g_seen_start = 0;
static time_t g_last_tick = 0;

typedef struct
{
	int offset;
	char file[128];
} mark_t;

static mark_t g_mark[13];

int InitShm()
{
	shmid = shmget(SHMKEY, SHMSIZE, IPC_CREAT | IPC_EXCL | 0666);
	if (-1 == shmid)
	{
		shmid = shmget(SHMKEY, SHMSIZE, 0666);
		if (-1 == shmid)
		{
			perror("shmget");
			return FAILURE;
		}
		shmctl(shmid, IPC_RMID, NULL);
		shmid = shmget(SHMKEY, SHMSIZE, IPC_CREAT | IPC_EXCL | 0666);
		if (-1 == shmid)
		{
			perror("shmget recreate");
			return FAILURE;
		}
	}

	g_addr = shmat(shmid, NULL, 0);
	if ((void *)-1 == g_addr || NULL == g_addr)
	{
		perror("shmat");
		g_addr = NULL;
		return FAILURE;
	}

	shm s;
	memset(&s, 0, sizeof(s));
	s.play_mode = SEQUENCEMODE;
	s.ppid = getpid();
	memcpy(g_addr, &s, sizeof(s));

	return SUCCESS;
}

int m_mp3_end(const char *name)
{
	const char *ptr = name;

	while (*ptr != '\0')
	{
		ptr++;
	}
	int i;
	for (i = 0; i < 4; i++)
	{
		ptr--;
	}

	if (ptr < name)
		return 0;

	return (strcmp(ptr, ".mp3") == 0) ? 1 : 0;
}

static void copy_root(const char *src)
{
	snprintf(g_root, sizeof(g_root), "%s", src);
	int len = strlen(g_root);
	if (len > 0 && g_root[len - 1] != '/' && len + 1 < (int)sizeof(g_root))
	{
		g_root[len] = '/';
		g_root[len + 1] = '\0';
	}
}

static int root_has_season(const char *root)
{
	char path[256];
	DIR *dir;

	snprintf(path, sizeof(path), "%s第1季", root);
	dir = opendir(path);
	if (NULL == dir)
	{
		return 0;
	}
	closedir(dir);
	return 1;
}

static void resolve_root(void)
{
	char tmp[128];

	copy_root(g_conf.music_path);
	if (root_has_season(g_root))
	{
		return;
	}

	snprintf(tmp, sizeof(tmp), "%s", g_root);
	copy_root("/sdcard/");
	if (root_has_season(g_root))
	{
		printf("music_path 下没有分季目录，改用 %s\n", g_root);
		return;
	}

	copy_root(tmp);
	printf("找不到第1季: %s第1季\n", g_root);
}

static void pos_path(char *out, int n)
{
	snprintf(out, n, "%speppa.pos", g_root[0] ? g_root : "/sdcard/");
}

static void trim_line(char *s)
{
	int len = strlen(s);
	while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
	{
		s[--len] = '\0';
	}
}

static void load_state(void)
{
	char path[256];
	char line[256];
	FILE *fp;

	pos_path(path, sizeof(path));
	fp = fopen(path, "r");
	if (NULL == fp)
	{
		return;
	}

	while (fgets(line, sizeof(line), fp))
	{
		int sea, off;
		char file[128];

		trim_line(line);
		if (strncmp(line, "cur=", 4) == 0)
		{
			g_resume_season = atoi(line + 4);
			continue;
		}

		file[0] = '\0';
		if (sscanf(line, "%d|%d|%127[^\n]", &sea, &off, file) != 3)
		{
			continue;
		}
		if (sea < 1 || sea > 12)
		{
			continue;
		}
		if (off < 0 || off > 1800)
		{
			off = 0;
		}
		g_mark[sea].offset = off;
		strncpy(g_mark[sea].file, file, sizeof(g_mark[sea].file) - 1);
		g_mark[sea].file[sizeof(g_mark[sea].file) - 1] = '\0';
	}

	fclose(fp);
}

static void save_state(void)
{
	char path[256];
	char tmp[256];
	FILE *fp;
	int i;

	if (g_season >= 1 && g_season <= 12 && g_track_name[0] != '\0')
	{
		g_mark[g_season].offset = g_track_sec;
		strncpy(g_mark[g_season].file, g_track_name, sizeof(g_mark[g_season].file) - 1);
		g_mark[g_season].file[sizeof(g_mark[g_season].file) - 1] = '\0';
		g_resume_season = g_season;
	}

	pos_path(path, sizeof(path));
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	fp = fopen(tmp, "w");
	if (NULL == fp)
	{
		return;
	}

	fprintf(fp, "cur=%d\n", g_resume_season);
	for (i = 1; i <= 12; i++)
	{
		if (g_mark[i].file[0] == '\0')
		{
			continue;
		}
		fprintf(fp, "%d|%d|%s\n", i, g_mark[i].offset, g_mark[i].file);
	}
	fclose(fp);
	rename(tmp, path);
}

static int cmp_name(const void *a, const void *b)
{
	return strcmp((const char *)a, (const char *)b);
}

static int name_in_list(const char *name)
{
	Node *p;

	if (NULL == head || NULL == name || name[0] == '\0')
	{
		return 0;
	}
	p = head->next;
	while (p != head)
	{
		if (strcmp(p->music_name, name) == 0)
		{
			return 1;
		}
		p = p->next;
	}
	return 0;
}

static int load_season_files(int season)
{
	char dir[256];
	DIR *dp;
	struct dirent *file;
	char (*names)[128];
	int count = 0;
	int i;

	if (season < 1 || season > 12 || g_root[0] == '\0')
	{
		return 0;
	}

	snprintf(dir, sizeof(dir), "%s第%d季", g_root, season);
	dp = opendir(dir);
	if (NULL == dp)
	{
		printf("打不开 %s\n", dir);
		return 0;
	}

	names = malloc(MAX_SONGS * 128);
	if (NULL == names)
	{
		closedir(dp);
		return 0;
	}

	while ((file = readdir(dp)) != NULL)
	{
		if (file->d_type != 8 && file->d_type != 0)
		{
			continue;
		}
		if (!m_mp3_end(file->d_name))
		{
			continue;
		}
		if (strlen(file->d_name) >= 128)
		{
			continue;
		}
		if (count >= MAX_SONGS)
		{
			printf("第%d季歌曲超过 %d 首，后面的先不放\n", season, MAX_SONGS);
			break;
		}
		strncpy(names[count], file->d_name, 127);
		names[count][127] = '\0';
		count++;
	}
	closedir(dp);

	if (count == 0)
	{
		printf("第%d季里没有 mp3\n", season);
		free(names);
		return 0;
	}

	qsort(names, count, 128, cmp_name);
	ClearLink();
	for (i = 0; i < count; i++)
	{
		if (FAILURE == InsertLink(head, names[i]))
		{
			printf("歌曲插入失败\n");
			free(names);
			return 0;
		}
	}
	free(names);

	snprintf(g_season_path, sizeof(g_season_path), "%s第%d季/", g_root, season);
	g_season = season;
	printf("第%d季 %d 首\n", season, count);
	return count;
}

static void begin_season(int season);
void play_music(const char *name);

static void arm_track(const char *name, int offset, int new_session)
{
	strncpy(g_track_name, name, sizeof(g_track_name) - 1);
	g_track_name[sizeof(g_track_name) - 1] = '\0';
	g_track_sec = offset;
	g_pending_offset = offset;
	g_seen_start = 0;
	g_since_save = 0;
	g_last_tick = 0;
	if (new_session)
	{
		g_session_sec = 0;
	}
}

static void apply_volume(void)
{
	int iLevel;

	iLeft = 80;
	iRight = 80;
	iLevel = (iRight << 8) | iLeft;
	if (g_mixerfd != -1 && ioctl(g_mixerfd, MIXER_WRITE(SOUND_MIXER_VOLUME), &iLevel) < 0)
		perror("设置音量");
	fprintf(stderr, "音量 %d\n", iLeft);
}

static void begin_season(int season)
{
	char name[128];
	int offset = 0;

	if (g_start_flag)
	{
		stop_play();
	}
	if (!load_season_files(season))
	{
		return;
	}

	if (g_mark[season].file[0] != '\0' && name_in_list(g_mark[season].file))
	{
		strncpy(name, g_mark[season].file, sizeof(name) - 1);
		name[sizeof(name) - 1] = '\0';
		offset = g_mark[season].offset;
	}
	else
	{
		strncpy(name, head->next->music_name, sizeof(name) - 1);
		name[sizeof(name) - 1] = '\0';
		offset = 0;
	}

	set_mode(SEQUENCEMODE);
	apply_volume();
	arm_track(name, offset, 1);
	printf("播放第%d季: %s", season, name);
	if (offset > 0)
	{
		printf("，从 %d 秒继续", offset);
	}
	printf("\n");
	play_music(name);
	g_start_flag = 1;
	g_suspend_flag = 0;
	save_state();
}

void init_library(void)
{
	int n;
	int season;

	memset(g_mark, 0, sizeof(g_mark));
	resolve_root();
	load_state();
	season = g_resume_season;
	if (season < 1 || season > 12)
	{
		season = 1;
	}
	n = load_season_files(season);
	if (n > 0)
	{
		printf("就绪: 第%d季 %d 首", season, n);
		if (g_mark[season].file[0] != '\0')
		{
			printf("，断点 %s @ %d秒", g_mark[season].file, g_mark[season].offset);
		}
		printf("\n");
		printf("服务器连不上也可以离线播放\n");
	}
}

void play_music(const char *name)
{
	int offset = g_pending_offset;
	pid_t child_pid;

	g_pending_offset = 0;
	child_pid = fork();
	if (-1 == child_pid)
	{
		perror("fork");
		exit(1);
	}
	else if (0 == child_pid)
	{
		int this_off = offset;
		int use_given = 1;

		while (1)
		{
			pid_t grand_pid = fork();
			if (-1 == grand_pid)
			{
				perror("fork");
				exit(1);
			}
			else if (0 == grand_pid)
			{
				char cur_name[128] = {0};
				shm s;
				int id = shmget(SHMKEY, SHMSIZE, 0);
				void *addr = NULL;
				char music_path[256] = {0};
				char offbuf[16];

				if (id != -1)
				{
					addr = shmat(id, NULL, 0);
				}
				if (addr == (void *)-1)
				{
					addr = NULL;
				}

				if (use_given && NULL != name && name[0] != '\0')
				{
					strncpy(cur_name, name, sizeof(cur_name) - 1);
				}
				else if (addr != NULL)
				{
					memcpy(&s, addr, sizeof(s));
					FindNextMusic(s.cur_name, SEQUENCEMODE, cur_name);
				}

				if (cur_name[0] == '\0' && head->next != head)
				{
					strncpy(cur_name, head->next->music_name, sizeof(cur_name) - 1);
				}

				if (addr != NULL)
				{
					memcpy(&s, addr, sizeof(s));
					strncpy(s.cur_name, cur_name, sizeof(s.cur_name) - 1);
					s.cur_name[sizeof(s.cur_name) - 1] = '\0';
					s.play_mode = SEQUENCEMODE;
					s.child_pid = getppid();
					s.grand_pid = getpid();
					memcpy(addr, &s, sizeof(s));
					shmdt(addr);
				}

				snprintf(music_path, sizeof(music_path), "%s%s", g_season_path, cur_name);
				printf("歌曲名字 %s\n", music_path);
				if (this_off > 0)
				{
					snprintf(offbuf, sizeof(offbuf), "%d", this_off);
					execl(g_conf.madplay_path, "madplay", "-s", offbuf, music_path, NULL);
				}
				else
				{
					execl(g_conf.madplay_path, "madplay", music_path, NULL);
				}
				perror("execl madplay");
				exit(1);
			}
			else
			{
				int status;
				waitpid(grand_pid, &status, 0);
				if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
				{
					sleep(1);
				}
				this_off = 0;
				use_given = 0;
			}
		}
	}
	else
	{
		return;
	}
}

void player_tick(void)
{
	time_t now;
	int delta;
	int limit;
	shm s;

	now = time(NULL);
	if (!g_start_flag || g_suspend_flag)
	{
		g_last_tick = now;
		return;
	}
	if (g_last_tick == 0)
	{
		g_last_tick = now;
	}
	delta = (int)(now - g_last_tick);
	g_last_tick = now;
	if (delta <= 0)
	{
		return;
	}
	if (delta > 5)
	{
		delta = 1;
	}

	memset(&s, 0, sizeof(s));
	if (NULL != g_addr)
	{
		memcpy(&s, g_addr, sizeof(s));
	}

	if (s.cur_name[0] != '\0' && g_seen_start && strcmp(s.cur_name, g_track_name) != 0)
	{
		strncpy(g_track_name, s.cur_name, sizeof(g_track_name) - 1);
		g_track_name[sizeof(g_track_name) - 1] = '\0';
		g_track_sec = delta;
		save_state();
		g_since_save = 0;
	}
	else
	{
		if (s.cur_name[0] != '\0' && strcmp(s.cur_name, g_track_name) == 0)
		{
			g_seen_start = 1;
		}
		g_track_sec += delta;
	}

	g_session_sec += delta;
	g_since_save += delta;

	limit = g_conf.play_minutes > 0 ? g_conf.play_minutes * 60 : 0;
	if (limit > 0 && g_session_sec >= limit)
	{
		printf("已播放 %d 分钟，自动停止\n", g_conf.play_minutes);
		stop_play();
		return;
	}
	if (g_since_save >= 10)
	{
		g_since_save = 0;
		save_state();
	}
}

void start_play()
{
	int season;

	if (g_start_flag == 1)
	{
		return;
	}
	season = g_resume_season;
	if (season < 1 || season > 12)
	{
		season = g_season > 0 ? g_season : 1;
	}
	begin_season(season);
}

void play_season(int season)
{
	if (season < 1 || season > 12)
	{
		return;
	}
	if (g_start_flag && g_season == season)
	{
		return;
	}
	begin_season(season);
}

void stop_play()
{
	shm s;

	if (g_start_flag == 0)
	{
		return;
	}

	save_state();
	g_start_flag = 0;
	g_suspend_flag = 0;

	memset(&s, 0, sizeof(s));
	if (NULL != g_addr)
	{
		memcpy(&s, g_addr, sizeof(s));
	}
	if (s.child_pid > 1)
	{
		kill(s.child_pid, SIGKILL);
	}
	if (s.grand_pid > 1)
	{
		kill(s.grand_pid, SIGKILL);
	}
}

void suspend_play()
{
	shm s;

	if (g_start_flag == 0 || g_suspend_flag == 1)
	{
		return;
	}

	save_state();
	memset(&s, 0, sizeof(s));
	memcpy(&s, g_addr, sizeof(s));

	if (s.grand_pid > 1)
	{
		kill(s.grand_pid, SIGSTOP);
	}
	if (s.child_pid > 1)
	{
		kill(s.child_pid, SIGSTOP);
	}

	g_suspend_flag = 1;
}

void continue_play()
{
	shm s;

	if (g_start_flag == 0)
	{
		start_play();
		return;
	}
	if (g_suspend_flag == 0)
	{
		return;
	}

	memset(&s, 0, sizeof(s));
	memcpy(&s, g_addr, sizeof(s));

	if (s.grand_pid > 1)
	{
		kill(s.grand_pid, SIGCONT);
	}
	if (s.child_pid > 1)
	{
		kill(s.child_pid, SIGCONT);
	}

	g_suspend_flag = 0;
	g_last_tick = 0;
}

void prior_play()
{
	char name[128] = {0};
	shm s;

	if (head == NULL || head->next == head)
	{
		start_play();
		return;
	}

	if (g_start_flag == 0)
	{
		strncpy(name, head->prior->music_name, sizeof(name) - 1);
		arm_track(name, 0, 1);
		apply_volume();
		play_music(name);
		g_start_flag = 1;
		g_suspend_flag = 0;
		save_state();
		return;
	}

	memset(&s, 0, sizeof(s));
	memcpy(&s, g_addr, sizeof(s));

	if (s.child_pid > 1)
	{
		kill(s.child_pid, SIGKILL);
	}
	if (s.grand_pid > 1)
	{
		kill(s.grand_pid, SIGKILL);
	}
	g_start_flag = 0;
	g_suspend_flag = 0;

	PriorMusic(s.cur_name[0] ? s.cur_name : g_track_name, SEQUENCEMODE, name);
	if (name[0] == '\0')
	{
		return;
	}
	arm_track(name, 0, 0);
	play_music(name);
	g_start_flag = 1;
	save_state();
}

void next_play()
{
	char name[128] = {0};
	shm s;

	if (head == NULL || head->next == head)
	{
		start_play();
		return;
	}

	if (g_start_flag == 0)
	{
		start_play();
		return;
	}

	memset(&s, 0, sizeof(s));
	memcpy(&s, g_addr, sizeof(s));

	if (s.child_pid > 1)
	{
		kill(s.child_pid, SIGKILL);
	}
	if (s.grand_pid > 1)
	{
		kill(s.grand_pid, SIGKILL);
	}
	g_start_flag = 0;
	g_suspend_flag = 0;

	NextMusic(s.cur_name[0] ? s.cur_name : g_track_name, SEQUENCEMODE, name);
	if (name[0] == '\0')
	{
		return;
	}
	arm_track(name, 0, 0);
	play_music(name);
	g_start_flag = 1;
	save_state();
}

void voice_up()
{
	int iLevel;

	if (iLeft < 100)
	{
		iLeft += 10;
	}
	if (iLeft > 100)
		iLeft = 100;
	iRight = iLeft;
	iLevel = (iRight << 8) | iLeft;
	if (g_mixerfd != -1 && ioctl(g_mixerfd, MIXER_WRITE(SOUND_MIXER_VOLUME), &iLevel) < 0)
		perror("设置音量");
	fprintf(stderr, "音量 %d\n", iLeft);
}

void voice_down()
{
	int iLevel;

	if (iLeft > 0)
	{
		iLeft -= 10;
	}
	if (iLeft < 0)
		iLeft = 0;
	iRight = iLeft;
	iLevel = (iRight << 8) | iLeft;
	if (g_mixerfd != -1 && ioctl(g_mixerfd, MIXER_WRITE(SOUND_MIXER_VOLUME), &iLevel) < 0)
		perror("设置音量");
	fprintf(stderr, "音量 %d\n", iLeft);
}

void set_mode(int mode)
{
	shm s;

	memset(&s, 0, sizeof(s));
	memcpy(&s, g_addr, sizeof(s));

	s.play_mode = mode;

	memcpy(g_addr, &s, sizeof(s));
	printf("模式修改成功!\n");
}

void single_play(const char *singer)
{
	char name[128] = {0};
	Node *p;

	if (NULL == singer || strlen(singer) == 0)
	{
		return;
	}

	stop_play();

	p = head->next;
	while (p != head)
	{
		if (strstr(p->music_name, singer) != NULL)
		{
			strncpy(name, p->music_name, sizeof(name) - 1);
			break;
		}
		p = p->next;
	}

	if (strlen(name) == 0)
	{
		printf("没有找到 %s 的歌\n", singer);
		return;
	}

	apply_volume();
	arm_track(name, 0, 1);
	play_music(name);
	g_start_flag = 1;
}
