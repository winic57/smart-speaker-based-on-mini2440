#ifndef DEVICE_H
#define DEVICE_H

#include <termios.h>

int InitDriver();
void led_on(int which);
void led_off(int which);
int InitSerial(const char *dev);
int get_key_id();

#endif
