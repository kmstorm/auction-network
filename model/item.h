#ifndef __ITEM__
#define __ITEM__

#define MAX_ITEMS 100
#define ITEM_FILE "items.txt"

typedef struct
{
  int id;
  int room_id;  // The auction room this item belongs to
  char name[50];
  char description[200];
  float starting_price;
  float current_price;   // Current highest bid price
  int highest_bidder;    // ID of the user with the highest bid
  int status; // 0: available, 1: sold
} Item;

/* Function Prototypes */
void load_items_from_file();
void save_item_to_file(const Item *item);
int create_item(int admin_id, int room_id, const char *name, const char *description, float starting_price);
int delete_item(int admin_id, int room_id, int item_id);
void list_items(int room_id);

#endif
