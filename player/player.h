#ifndef PLAYER_H
#define PLAYER_H

#include <unistd.h>
#include "config.h"

#define SHMKEY     1234
#define SHMSIZE    4096

#define SEQUENCEMODE    1
#define RANDOM          2
#define CIRCLE          3

//共享内存数据
struct shm
{
	int play_mode;
	char cur_name[128];
	pid_t ppid;
	pid_t child_pid;
	pid_t grand_pid;
};
typedef struct shm shm;

int InitShm();
void init_library();
void player_tick();
void play_season(int season);
void start_play();
void stop_play();
void suspend_play();
void continue_play();
void prior_play();
void next_play();
void voice_up();
void voice_down();
void set_mode(int mode);
void single_play(const char *singer);


#endif
