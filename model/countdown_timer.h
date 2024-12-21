#ifndef __COUNTDOWN_TIMER_H__
#define __COUNTDOWN_TIMER_H__

#include <time.h>

void countdown_to_start_time(int sock, const char *start_time);
void countdown_room_duration(int sock, const char *start_time, int duration);

#endif
