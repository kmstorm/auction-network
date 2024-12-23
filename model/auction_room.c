#include "auction_room.h"
#include "countdown_timer.h"
#include "item.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Global variables */
static AuctionRoom rooms[MAX_ROOMS];
static int room_count = 0;

/* Load rooms from file */
void load_rooms_from_file()
{
  FILE *file = fopen(ROOM_FILE, "r");
  if (!file)
  {
    printf("No room file found, starting fresh.\n");
    return;
  }

  while (fscanf(file, "%d %49s %199s %d %19s %d\n", &rooms[room_count].id, rooms[room_count].name, rooms[room_count].description,
                &rooms[room_count].status, rooms[room_count].start_time, &rooms[room_count].duration) != EOF)
  {
    room_count++;
  }

  fclose(file);
}

/* Save room to file */
void save_room_to_file(const AuctionRoom *room)
{
  FILE *file = fopen(ROOM_FILE, "a");
  if (!file)
  {
    perror("Failed to open room file");
    return;
  }

  fprintf(file, "%d %s %s %d %s %d\n", room->id, room->name, room->description, room->status, room->start_time, room->duration);
  fclose(file);
}

/* Save all rooms to file */
void save_all_rooms_to_file()
{
  FILE *file = fopen(ROOM_FILE, "w");
  if (!file)
  {
    perror("Failed to open room file");
    return;
  }

  for (int i = 0; i < room_count; i++)
  {
    fprintf(file, "%d %s %s %d %s %d\n", rooms[i].id, rooms[i].name, rooms[i].description, rooms[i].status, rooms[i].start_time, rooms[i].duration);
  }

  fclose(file);
}

AuctionRoom* get_room_by_id(int room_id)
{
    for (int i = 0; i < room_count; i++)
    {
        if (rooms[i].id == room_id)
        {
            return &rooms[i];
        }
    }
    return NULL;
}

/* Create a new room - only admin can do this */
int create_room(int admin_id, const char *name, const char *description, const char *start_time, int duration)
{
  printf("Log_CREATE_ROOM: create_room called with admin_id=%d, name=%s, description=%s, start_time=%s, duration=%d\n", admin_id, name, description, start_time, duration);

  if (admin_id != 1) // Assuming 1 is admin id
  {
    printf("Log_CREATE_ROOM: Only admin can create rooms\n");
    return 0; // Only admin can create rooms
  }
  load_rooms_from_file();
  if (room_count < MAX_ROOMS)
  {
    AuctionRoom new_room;
    new_room.id = room_count + 1;
    strcpy(new_room.name, name);
    strcpy(new_room.description, description);
    strcpy(new_room.start_time, start_time);
    new_room.duration = duration;
    new_room.status = 0; // Room is not started yet

    save_room_to_file(&new_room);
    rooms[room_count++] = new_room;
    printf("Log_CREATE_ROOM: Room created successfully with id=%d\n", new_room.id);
    return 1;
  }

  printf("Log_CREATE_ROOM: Room creation failed, max rooms reached\n");
  return 0;
}

/* Delete room - only admin can do this */
int delete_room(int admin_id, int room_id)
{
  if (admin_id != 1) // Only admin can delete rooms
  {
    return 0;
  }
  load_rooms_from_file();
  for (int i = 0; i < room_count; i++)
  {
    if (rooms[i].id == room_id)
    {
      // Shift the rooms down
      for (int j = i; j < room_count - 1; j++)
      {
        rooms[j] = rooms[j + 1];
      }
      room_count--;
      save_all_rooms_to_file(); // Save updated rooms to file
      return 1;
    }
  }

  return 0;
}

/* List all rooms */
void list_rooms(char *room_list, size_t room_list_size)
{
    room_list[0] = '\0'; // Initialize the room list string
    for (int i = 0; i < room_count; i++)
    {
        char room_info[256];
        snprintf(room_info, sizeof(room_info), "ID: %d, Name: %s, Status: %d, Start: %s, Duration: %d\n",
                rooms[i].id, rooms[i].name, rooms[i].status, rooms[i].start_time, rooms[i].duration);
        strncat(room_list, room_info, room_list_size - strlen(room_list) - 1);
    }
}

int has_room_started(int room_id)
{
    for (int i = 0; i < room_count; i++)
    {
        if (rooms[i].id == room_id)
        {
            struct tm tm = {0};
            time_t start_epoch, current_epoch;

            if (strptime(rooms[i].start_time, "%Y-%m-%dT%H:%M", &tm) == NULL)
            {
                fprintf(stderr, "Error parsing start_time: %s\n", rooms[i].start_time);
                return 0;
            }

            start_epoch = mktime(&tm);
            time(&current_epoch);

            printf("LOG_HAS_ROOM_STARTED: Start time: %ld, Current time: %ld\n", start_epoch, current_epoch);

            return difftime(current_epoch, start_epoch) >= 0;
        }
    }
    return 0;
}

/* Join a room */
int join_room(int user_id, int room_id)
{
    printf("Log_JOIN_ROOM: join_room called with user_id=%d, room_id=%d\n", user_id, room_id);
    if (user_id == 0) // Assuming 0 is a guest or invalid user
    {
        return 0; // User must be logged in to join a room
    }

    // Check if user is already in another room
    for (int i = 0; i < room_count; i++)
    {
        for (int j = 0; j < MAX_ROOMS; j++)
        {
            if (strcmp(rooms[i].participants[j], "") != 0 && user_id == atoi(rooms[i].participants[j]))
            {
                return 0; // User is already in another room
            }
        }
    }

    for (int i = 0; i < room_count; i++)
    {
        if (rooms[i].id == room_id)
        {
            for (int j = 0; j < MAX_ROOMS; j++)
            {
                if (strcmp(rooms[i].participants[j], "") == 0)
                {
                    snprintf(rooms[i].participants[j], 50, "%d", user_id);
                    printf("Log_JOIN_ROOM: User %d joined room %d\n", user_id, room_id);
                    return 1;
                }
            }
        }
    }

    printf("Log_JOIN_ROOM: Room %d not found or full\n", room_id);
    return 0;
}

/* Leave a room */
int leave_room(int user_id, int room_id)
{
    printf("Log_LEAVE_ROOM: leave_room called with user_id=%d, room_id=%d\n", user_id, room_id);
    for (int i = 0; i < room_count; i++)
    {
        if (rooms[i].id == room_id)
        {
            for (int j = 0; j < MAX_ROOMS; j++)
            {
                if (strcmp(rooms[i].participants[j], "") != 0 && user_id == atoi(rooms[i].participants[j]))
                {
                    strcpy(rooms[i].participants[j], "");
                    printf("Log_LEAVE_ROOM: User %d left room %d\n", user_id, room_id);
                    return 1;
                }
            }
        }
    }
    printf("Log_LEAVE_ROOM: User %d not found in room %d\n", user_id, room_id);
    return 0;
}

void list_room_items(int room_id)
{
  list_items(room_id);
}

/* Create an item in the room - only admin can do this */
int create_item_in_room(int admin_id, int room_id, const char *name, const char *description, float starting_price)
{
  return create_item(admin_id, room_id, name, description, starting_price);
}

/* Delete an item from the room - only admin can do this */
int delete_item_from_room(int admin_id, int room_id, int item_id)
{
  return delete_item(admin_id, room_id, item_id);
}

int process_bid(int user_id, int room_id, float bid_amount)
{
    // Gọi hàm find_item để tìm vật phẩm đang đấu giá trong room
    Item *item = find_item(room_id);
    if (!item)
    {
        printf("LOG_PROCESS_BID: No available items in Room ID %d\n", room_id);
        return 0; // Không có vật phẩm nào để đấu giá
    }

    if (bid_amount >= item->buy_now_price)
    {
        item->current_price = item->buy_now_price;
        item->highest_bidder = user_id;
        item->status = 1; // Vật phẩm được bán
        save_all_items_to_file();
        printf("LOG_PROCESS_BID: Item ID %d in Room ID %d sold immediately to User %d for %.2f\n", 
               item->id, room_id, user_id, item->buy_now_price);
        return 1; // Vật phẩm được bán
    }

    if (bid_amount > item->current_price)
    {
        item->current_price = bid_amount;
        item->highest_bidder = user_id;
        save_all_items_to_file();
        printf("LOG_PROCESS_BID: Bid accepted - User %d is now the highest bidder for Item ID %d in Room ID %d with amount %.2f\n",
               user_id, item->id, room_id, bid_amount);
        return 1; // Bid được chấp nhận
    }

    printf("LOG_PROCESS_BID: Bid rejected - Bid amount %.2f is less than current price %.2f for Item ID %d in Room ID %d\n",
           bid_amount, item->current_price, item->id, room_id);
    return 0; // Bid bị từ chối
}
