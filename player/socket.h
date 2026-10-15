#ifndef SOCKET_H
#define SOCKET_H

extern volatile int g_link_down;

int InitSocket();
void socket_start_play();
void socket_stop_play();
void socket_suspend_play();
void socket_continue_play();
void socket_prior_play();
void socket_next_play();
void socket_voice_up_play();
void socket_voice_down_play();
void socket_mode_play(int);
void socket_get_status();
void socket_get_music();

#endif
