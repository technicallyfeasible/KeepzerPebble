#include "pebble.h"
#include "mini-printf.h"
#include "storage.h"
#include "display.h"

char s_key_token[128] = "\0";
char s_sensor_id[64];
ActivityItem s_activity_items[MAX_ACTIVITY_ITEMS];
int s_active_item_count = 0;
int current_item = 0;

/* queue for items being logged */
LogItem s_log_items[MAX_LOG_ITEMS];
int s_log_item_count;

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
}
/* Store the keytoken */
void store_keytoken() {
	persist_write_string(STORAGE_KEYTOKEN, s_key_token);
}
/* Load the keytoken */
void load_keytoken() {
	persist_read_string(STORAGE_KEYTOKEN, s_key_token, sizeof(s_key_token));
}
/* Store all log items in persistent storage */
void store_log() {
	char text[32];
	mini_snprintf(text, sizeof(text), "%d events pending", s_log_item_count);
	APP_LOG(APP_LOG_LEVEL_DEBUG, text);
	
	persist_write_int(STORAGE_LOG_COUNT, s_log_item_count);
	persist_write_data(STORAGE_LOGS, s_log_items, sizeof(LogItem)*s_log_item_count);
}
/* Load all log items from persistent storage */
void load_log() {
	s_log_item_count = persist_read_int(STORAGE_LOG_COUNT);
    if (s_log_item_count > MAX_LOG_ITEMS)
	  s_log_item_count = MAX_LOG_ITEMS;
	persist_read_data(STORAGE_LOGS, s_log_items, sizeof(LogItem)*s_log_item_count);

	char text[32];
	mini_snprintf(text, sizeof(text), "%d events pending", s_log_item_count);
	APP_LOG(APP_LOG_LEVEL_DEBUG, text);
}


void activity_append(char *name, char* type, char* json) {
	if (s_active_item_count >= MAX_ACTIVITY_ITEMS) { 
		return;
	}
	strcpy(s_activity_items[s_active_item_count].name, name);
	strcpy(s_activity_items[s_active_item_count].type, type);
	strcpy(s_activity_items[s_active_item_count].json, json);
	s_active_item_count++;
}
void activity_set(int index, char *name, char* type, char* json) {
	if (index >= s_active_item_count || index >= MAX_ACTIVITY_ITEMS) { 
		return;
	}
	strcpy(s_activity_items[index].name, name);
	strcpy(s_activity_items[index].type, type);
	strcpy(s_activity_items[index].json, json);
}

void logitem_append(int index, char* dateString) {
	if (s_log_item_count >= MAX_ACTIVITY_ITEMS || index >= s_active_item_count) { 
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Cannot append log item.");
		return;
	}
	strcpy(s_log_items[s_log_item_count].date, dateString);
	strcpy(s_log_items[s_log_item_count].type, s_activity_items[index].type);
	strcpy(s_log_items[s_log_item_count].json, s_activity_items[index].json);
	s_log_item_count++;

	store_log();
	display_update_state();
}
void logitem_remove(int index) {
	if (s_log_item_count == 0 || index >= s_active_item_count) { 
		return;
	}
	int i;
	for (i = index + 1; i < s_log_item_count; i++)
		s_log_items[i - 1] = s_log_items[i];
	s_log_item_count--;

	store_log();
	display_update_state();
}

void set_keytoken(char *data) {
	strcpy(s_key_token, data);
}

void set_sensorid(char *data) {
	strcpy(s_sensor_id, data);
}
