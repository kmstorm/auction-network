#ifndef __AUCTION_ROOM__
#define __AUCTION_ROOM__

#include <stddef.h>

#define MAX_ROOMS 10
#define ROOM_FILE "rooms.txt"

typedef struct
{
  int id;
  char name[50];
  char description[200];
  int status; // 0: not started, 1: ongoing, 2: ended
  char start_time[20];
  int duration; // Duration in minutes
  char participants[MAX_ROOMS][50]; // List of participants (usernames)
} AuctionRoom;

/* Function */
void load_rooms_from_file();
void save_room_to_file(const AuctionRoom *room);
void save_all_rooms_to_file();
AuctionRoom* get_room_by_id(int room_id);
int create_room(int admin_id, const char *name, const char *description, const char *start_time, int duration);
int delete_room(int admin_id, int room_id);
void list_rooms(char *room_list, size_t room_list_size);
int has_room_started(int room_id);
int join_room(int user_id, int room_id);
int leave_room(int user_id, int room_id);
void list_room_items(int room_id);
int process_bid(int user_id, int room_id, float bid_amount);

#endif
