#include "item.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

  printf("Log_LOAD_ITEMS: Reading items from file\n");
  while (fscanf(file, "%d %d %49s %199s %f %f %d %d\n", 
              &items[item_count].id, &items[item_count].room_id, 
              items[item_count].name, items[item_count].description, 
              &items[item_count].starting_price, &items[item_count].current_price, 
              &items[item_count].highest_bidder, &items[item_count].status) != EOF)
  {
      item_count++;
      if (item_count >= MAX_ITEMS)
      {
          printf("Log_LOAD_ITEMS: Maximum item count reached\n");
          break;
      }
  }

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

  fprintf(file, "%d %d %s %s %.2f %.2f %d %d\n", 
            item->id, item->room_id, item->name, item->description, 
            item->starting_price, item->current_price, 
            item->highest_bidder, item->status);
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
    fprintf(file, "%d %d %s %s %.2f %.2f %d %d\n", 
        items[i].id, items[i].room_id, items[i].name, items[i].description, 
        items[i].starting_price, items[i].current_price, 
        items[i].highest_bidder, items[i].status);
  }

  fclose(file);
}

/* Create a new item - only admin can do this */
int create_item(int admin_id, int room_id, const char *name, const char *description, float starting_price)
{
  printf("Log_CREATE_ITEM: create_item called with admin_id=%d, room_id=%d, name=%s, description=%s, starting_price=%.2f\n", 
          admin_id, room_id, name, description, starting_price);

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
      strcpy(new_item.description, description);
      new_item.starting_price = starting_price;
      new_item.current_price = starting_price; // Current price starts with starting price
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
void list_items(int room_id)
{
    int item_found = 0;
    for (int i = 0; i < item_count; i++)
    {
        if (items[i].room_id == room_id)  // Only list items for the specified room
        {
            printf("ID: %d, Name: %s, Price: %.2f, Status: %s\n",
                  items[i].id, items[i].name, items[i].starting_price,
                  items[i].status == 0 ? "Available" : "Sold");
            item_found = 1;
        }
    }

    if (!item_found)
    {
        printf("No items found for room ID %d\n", room_id);  // If no items are found in the room
    }
}

Item* get_item_by_id(int room_id, int item_id)
{
    for (int i = 0; i < item_count; i++)
    {
        if (items[i].room_id == room_id && items[i].id == item_id)
        {
            return &items[i];
        }
    }
    return NULL;
}
