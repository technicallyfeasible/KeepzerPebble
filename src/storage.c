#include "pebble.h"
#include "mini-printf.h"
#include "storage.h"

char s_key_token[128] = "\0";
ActivityItem s_activity_items[MAX_ACTIVITY_ITEMS];
int s_active_item_count = 0;
int current_item = 0;

/* Store all activities in persistent storage */
void store_config() {
	persist_write_int(STORAGE_ITEM_CURRENT, current_item);
	persist_write_int(STORAGE_ITEM_COUNT, s_active_item_count);
	persist_write_data(STORAGE_ITEMS, s_activity_items, sizeof(ActivityItem)*s_active_item_count);
}
/* Load all activities in persistent storage */
void load_config() {
	s_active_item_count = persist_read_int(STORAGE_ITEM_COUNT);
	current_item = persist_read_int(STORAGE_ITEM_CURRENT);
	if (current_item >= s_active_item_count)
		current_item = s_active_item_count;
	if (s_active_item_count == 0)
		return;
    if (s_active_item_count > MAX_ACTIVITY_ITEMS)
	  s_active_item_count = MAX_ACTIVITY_ITEMS;
	persist_read_data(STORAGE_ITEMS, s_activity_items, sizeof(ActivityItem)*s_active_item_count);
	
	// get items from the configuration
	if(s_active_item_count == 0)
	{
		
	}
}
/* Store the keytoken */
void store_keytoken() {
	persist_write_string(STORAGE_KEYTOKEN, s_key_token);
}
/* Load the keytoken */
void load_keytoken() {
	persist_read_string(STORAGE_KEYTOKEN, s_key_token, sizeof(s_key_token));
}


void activity_append(char *data) {
	if (s_active_item_count >= MAX_ACTIVITY_ITEMS) { 
		return;
	}
	strcpy(s_activity_items[s_active_item_count].name, data);
	s_active_item_count++;
}
void activity_set(int index, char *data) {
	if (index >= s_active_item_count || index >= MAX_ACTIVITY_ITEMS) { 
		return;
	}
	strcpy(s_activity_items[index].name, data);
}
