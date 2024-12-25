#ifndef __COUNTDOWN_TIMER_H__
#define __COUNTDOWN_TIMER_H__

#include <time.h>
#include <stdlib.h>

typedef struct {
    int sock;
    int room_id;
    int duration;
} CountdownArgs;

void countdown_to_start_time(int sock, const char *start_time);
void countdown_room_duration(int sock, int room_id, int duration);
void* countdown_thread(void* arg);

#endif
