#ifndef __AUCTION_ROOM__
#define __AUCTION_ROOM__

#define MAX_ROOMS 100
#define ROOM_FILE "rooms.txt"

typedef struct
{
  int id;
  char name[50];
  char description[200];
  int status; // 0: not started, 1: ongoing, 2: ended
  char start_time[20];
  char end_time[20];
  char participants[MAX_ROOMS][50]; // List of participants (usernames)
} AuctionRoom;

/* Function */
void load_rooms_from_file();
void save_room_to_file(const AuctionRoom *room);
int create_room(int admin_id, const char *name, const char *description, const char *start_time, const char *end_time);
int delete_room(int admin_id, int room_id);
void list_rooms();
int join_room(int user_id, int room_id);
int leave_room(int user_id, int room_id);
void list_room_items(int room_id);
int create_item_in_room(int admin_id, int room_id, const char *name, const char *description, float starting_price);
int delete_item_from_room(int admin_id, int room_id, int item_id);
#endif
