#include "item.h"
#include "auction_room.h"
#include "message.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

static Item items[MAX_ITEMS];
static int item_count = 0;

/* Load items from file */
void load_items_from_file()
{
  printf("Log_LOAD_ITEMS: Opening item file\n");
  FILE *file = fopen(ITEM_FILE, "r");
  if (!file)
  {
    printf("Log_LOAD_ITEMS: No item file found, starting fresh.\n");
    return;
  }

  // Reset item count and clear items array
  item_count = 0;
  memset(items, 0, sizeof(items));

  printf("Log_LOAD_ITEMS: Reading items from file\n");
  while (fscanf(file, "%d %d %49s %f %f %f %d\n",
                &items[item_count].id, &items[item_count].room_id,
                items[item_count].name,
                &items[item_count].starting_price, &items[item_count].current_price,
                &items[item_count].buy_now_price, &items[item_count].status) != EOF)
  {

    item_count++;
    if (item_count >= MAX_ITEMS)
    {
      printf("Log_LOAD_ITEMS: Maximum item count reached\n");
      break;
    }
  }

  printf("Log_LOAD_ITEMS: Total items loaded: %d\n", item_count);
  fclose(file);
  printf("Log_LOAD_ITEMS: Finished loading items\n");
}

/* Save item to file */
void save_item_to_file(const Item *item)
{
  FILE *file = fopen(ITEM_FILE, "a");
  if (!file)
  {
    perror("Failed to open item file");
    return;
  }

  fprintf(file, "%d %d %s %.2f %.2f %.2f %d\n",
          item->id, item->room_id, item->name,
          item->starting_price, item->current_price,
          item->buy_now_price, item->status);
  fclose(file);
}

/* Save all items to file */
void save_all_items_to_file()
{
  FILE *file = fopen(ITEM_FILE, "w");
  if (!file)
  {
    perror("Failed to open item file");
    return;
  }

  for (int i = 0; i < item_count; i++)
  {
    fprintf(file, "%d %d %s %.2f %.2f %.2f %d\n",
                items[i].id, items[i].room_id, items[i].name,
                items[i].starting_price, items[i].current_price,
                items[i].buy_now_price, items[i].status);
  }

  fclose(file);
  printf("LOG_SAVE_ITEMS: All items have been saved to file.\n");
}

/* Create a new item - only admin can do this */
int create_item(int admin_id, int room_id, const char *name, float starting_price, float buy_now_price)
{
  printf("Log_CREATE_ITEM: create_item called with admin_id=%d, room_id=%d, name=%s, starting_price=%.2f, buy_now_price=%.2f\n",
          admin_id, room_id, name, starting_price, buy_now_price);

  if (admin_id != 1) // Only admin (id 1) can create items
  {
      printf("Log_CREATE_ITEM: Only admin can create items\n");
      return 0; // Permission denied
  }

  if (item_count < MAX_ITEMS)
  {
      Item new_item;
      new_item.id = item_count + 1;
      new_item.room_id = room_id;
      strcpy(new_item.name, name);
      new_item.starting_price = starting_price;
      new_item.current_price = starting_price; // Current price starts with starting price
      new_item.buy_now_price = buy_now_price;
      new_item.highest_bidder = -1;           // No bidder yet
      new_item.status = 0;                    // Available

      save_item_to_file(&new_item);           // Save the new item to file
      items[item_count++] = new_item;         // Add item to the in-memory list

      printf("Log_CREATE_ITEM: Item created successfully with id=%d\n", new_item.id);
      return 1; // Success
  }

  printf("Log_CREATE_ITEM: Item creation failed, max items reached\n");
  return 0;
}

/* Delete item - only admin can do this */
int delete_item(int admin_id, int room_id, int item_id)
{
  if (admin_id != 1) // Only admin can delete items
  {
    printf("Log_DELETE_ITEM: Only admin can delete items\n");
    return 0;
  }

  for (int i = 0; i < item_count; i++)
  {
    if (items[i].room_id == room_id && items[i].id == item_id)
    {
      // Shift the items down to delete the item
      for (int j = i; j < item_count - 1; j++)
      {
        items[j] = items[j + 1];
      }
      item_count--;
      save_all_items_to_file(); // Save updated items to file
      printf("Log_DELETE_ITEM: Item with id %d deleted successfully\n", item_id);
      return 1; // Item successfully deleted
    }
  }

  printf("Log_DELETE_ITEM: Item with id %d not found\n", item_id);
  return 0; // Item not found
}

/* List all items in a specific room */
void list_items(int room_id, char *item_list, size_t item_list_size)
{
    item_list[0] = '\0'; // Initialize the item list string
    int item_found = 0;

    for (int i = 0; i < item_count; i++)
    {
        if (items[i].room_id == room_id) // Only list items for the specified room
        {
            char item_info[256];
            snprintf(item_info, sizeof(item_info),
                     "ID: %d, Name: %s, Starting Price: %.2f, Buy Now Price: %.2f, Status: %s\n",
                     items[i].id, items[i].name, items[i].starting_price, items[i].buy_now_price,
                     items[i].status == 0 ? "Available" : "Sold");
            strncat(item_list, item_info, item_list_size - strlen(item_list) - 1);
            item_found = 1;
        }
    }

    if (!item_found)
    {
        snprintf(item_list, item_list_size, "No items found for room ID %d\n", room_id);
    }
}

Item* find_item(int room_id)
{
    for (int i = 0; i < item_count; i++)
    {
        if (items[i].room_id == room_id && items[i].status == 0)
        {
            return &items[i]; // Trả về con trỏ đến vật phẩm đầu tiên có trạng thái "Available"
        }
    }
    return NULL; // Không tìm thấy vật phẩm nào
}

/* Search items by keyword and time range */
void search_items(int sock, const char *keyword, const char *start_time, const char *end_time)
{
    int item_found = 0;
    char result[BUFFER_SIZE] = "";

    printf("Log_SEARCH_ITEMS: Starting search with keyword='%s', start_time='%s', end_time='%s'\n", keyword, start_time, end_time);

    for (int i = 0; i < item_count; i++)
    {
        AuctionRoom *room = get_room_by_id(items[i].room_id);
        if (!room)
        {
            printf("Log_SEARCH_ITEMS: Room with ID %d not found for item ID %d\n", items[i].room_id, items[i].id);
            continue;
        }

        int keyword_match = keyword && strstr(items[i].name, keyword);
        int time_match = 1;

        if (start_time[0] != '\0' && end_time[0] != '\0')
        {
            time_match = strcmp(room->start_time, start_time) >= 0 && strcmp(room->start_time, end_time) <= 0;
        }

        printf("Log_SEARCH_ITEMS: Checking item ID %d, keyword_match=%d, time_match=%d\n", items[i].id, keyword_match, time_match);

        if (keyword_match && time_match)
        {
            char item_info[256];
            snprintf(item_info, sizeof(item_info),
                     "\nRoom ID: %d, Item ID: %d, Name: %s, Start Time: %s, Current Price: %.2f, Buy Now Price: %.2f, Status: %s\n",
                     items[i].room_id, items[i].id, items[i].name, room->start_time,
                     items[i].current_price, items[i].buy_now_price, items[i].status == 0 ? "Available" : "Sold");
            strncat(result, item_info, sizeof(result) - strlen(result) - 1);
            item_found = 1;
        }
    }

    if (!item_found)
    {
        snprintf(result, sizeof(result), "No items found matching the criteria.\n");
    }

    printf("Log_SEARCH_ITEMS: Search result: %s\n", result);

    Message message;
    build_message(&message, 13, result);
    send(sock, &message, sizeof(Message), 0);
}
