#ifndef __ITEM__
#define __ITEM__

#define MAX_ITEMS 100
#define ITEM_FILE "items.txt"

typedef struct
{
  int id;
  int room_id;          // Room this item belongs to
  char name[50];
  char description[200];
  float starting_price;  // Initial price of the item
  float current_price;   // Current highest bid price
  float buy_now_price;   // Price to buy immediately
  int highest_bidder;    // ID of the highest bidder
  int status;            // 0: Available, 1: Sold
} Item;

/* Function Prototypes */
void load_items_from_file();
void save_item_to_file(const Item *item);
void save_all_items_to_file();
int create_item(int admin_id, int room_id, const char *name, const char *description, float starting_price, float buy_now_price);
int delete_item(int admin_id, int room_id, int item_id);
void list_items(int room_id);
Item* find_item(int room_id);
void search_items(int sock, const char *keyword, const char *start_time, const char *end_time);

#endif
