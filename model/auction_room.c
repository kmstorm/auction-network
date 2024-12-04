#include "auction_room.h"
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

  while (fscanf(file, "%d %49s %199s %d %19s %19s\n", &rooms[room_count].id, rooms[room_count].name, rooms[room_count].description,
                &rooms[room_count].status, rooms[room_count].start_time, rooms[room_count].end_time) != EOF)
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

  fprintf(file, "%d %s %s %d %s %s\n", room->id, room->name, room->description, room->status, room->start_time, room->end_time);
  fclose(file);
}

/* Create a new room - only admin can do this */
int create_room(int admin_id, const char *name, const char *description, const char *start_time, const char *end_time)
{
  if (admin_id != 1) // Assuming 1 is admin id
  {
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
    strcpy(new_room.end_time, end_time);
    new_room.status = 0; // Room is not started yet

    save_room_to_file(&new_room);
    rooms[room_count++] = new_room;
    return 1;
  }

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
      return 1;
    }
  }

  return 0;
}

/* List all rooms */
void list_rooms()
{
  load_rooms_from_file();
  for (int i = 0; i < room_count; i++)
  {
    printf("ID: %d, Name: %s, Status: %d, Start: %s, End: %s\n", rooms[i].id, rooms[i].name, rooms[i].status,
           rooms[i].start_time, rooms[i].end_time);
  }
}

/* Join a room */
int join_room(int user_id, int room_id)
{
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
          return 1;
        }
      }
    }
  }

  return 0;
}

/* Leave a room */
int leave_room(int user_id, int room_id)
{
  for (int i = 0; i < room_count; i++)
  {
    if (rooms[i].id == room_id)
    {
      for (int j = 0; j < MAX_ROOMS; j++)
      {
        if (strcmp(rooms[i].participants[j], "") != 0 && user_id == atoi(rooms[i].participants[j]))
        {
          strcpy(rooms[i].participants[j], "");
          return 1;
        }
      }
    }
  }

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
