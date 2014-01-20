/*
 * Storage stuff
 */


#ifndef __KEEPZER_STORAGE__
#define __KEEPZER_STORAGE__

#define MAX_ACTIVITY_ITEMS (20)
#define MAX_ITEM_TEXT_LENGTH (32)

#define STORAGE_ITEM_COUNT 0
#define STORAGE_ITEMS 1
#define STORAGE_ITEM_CURRENT 2
#define STORAGE_KEYTOKEN 3

extern char s_key_token[128];
typedef struct {
  char name[MAX_ITEM_TEXT_LENGTH];
} ActivityItem;
extern ActivityItem s_activity_items[MAX_ACTIVITY_ITEMS];
extern int s_active_item_count;
extern int current_item;

/* Store all activities in persistent storage */
void store_config();
/* Load all activities in persistent storage */
void load_config();
/* Store the keytoken */
void store_keytoken();
/* Load the keytoken */
void load_keytoken();

void activity_append(char *data);
void activity_set(int index, char *data);
void set_keytoken(char *data);

#endif