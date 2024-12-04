#include "item.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static Item items[MAX_ITEMS];
static int item_count = 0;

/* Load items from file */
void load_items_from_file()
{
  FILE *file = fopen(ITEM_FILE, "r");
  if (!file)
  {
    printf("No item file found, starting fresh.\n");
    return;
  }

  while (fscanf(file, "%d %d %49s %199s %f %d\n", &items[item_count].id, &items[item_count].room_id, items[item_count].name, items[item_count].description,
                 &items[item_count].starting_price, &items[item_count].status) != EOF)
  {
    item_count++;
  }

  fclose(file);
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

  fprintf(file, "%d %d %s %s %f %d\n", item->id, item->room_id, item->name, item->description, item->starting_price, item->status);
  fclose(file);
}

/* Create a new item - only admin can do this */
int create_item(int admin_id, int room_id, const char *name, const char *description, float starting_price)
{
  if (admin_id != 1) // Only admin (id 1) can create items
  {
    return 0; // Permission denied
  }

  load_items_from_file();
  if (item_count < MAX_ITEMS)
  {
    Item new_item;
    new_item.id = item_count + 1;
    new_item.room_id = room_id;
    strcpy(new_item.name, name);
    strcpy(new_item.description, description);
    new_item.starting_price = starting_price;
    new_item.status = 0; // Available

    save_item_to_file(&new_item);
    items[item_count++] = new_item;
    return 1;
  }

  return 0;
}

/* Delete item - only admin can do this */
int delete_item(int admin_id, int room_id, int item_id)
{
  if (admin_id != 1) // Only admin can delete items
  {
    return 0;
  }

  load_items_from_file();
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
      return 1; // Item successfully deleted
    }
  }

  return 0; // Item not found
}

/* List all items in a specific room */
void list_items(int room_id)
{
    load_items_from_file();  // Load the items from the file before listing them

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
