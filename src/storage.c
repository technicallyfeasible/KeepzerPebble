#include "pebble.h"
#include "storage.h"
#include "display.h"
#include "connect.h"

char s_key_token[164] = "\0";
char s_sensor_id[32];
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
	// each item has to be written separately because maximum value size is 256 bytes
	for(int i = 0; i < s_active_item_count; i++) {
		persist_write_data(STORAGE_ITEMS + i, &s_activity_items[i], sizeof(ActivityItem));
	}
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

	// read each item separately
	for(int i = 0; i < s_active_item_count; i++) {
		persist_read_data(STORAGE_ITEMS + i, &s_activity_items[i], sizeof(ActivityItem));
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
/* Store all log items in persistent storage */
void store_log() {
	persist_write_int(STORAGE_LOG_COUNT, s_log_item_count);
	// each item has to be written separately because maximum value size is 256 bytes
	for(int i = 0; i < s_log_item_count; i++) {
		persist_write_data(STORAGE_LOGS + i, &s_log_items[i], sizeof(LogItem));
	}
}
/* Load all log items from persistent storage */
void load_log() {
	s_log_item_count = persist_read_int(STORAGE_LOG_COUNT);
    if (s_log_item_count > MAX_LOG_ITEMS)
	  s_log_item_count = MAX_LOG_ITEMS;
	// load log items separately
	for(int i = 0; i < s_log_item_count; i++) {
		persist_read_data(STORAGE_LOGS + i, &s_log_items[i], sizeof(LogItem));
	}
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

/* copy a string to the destination and escape double quotes */
void safestrcpy(char *dest, const char *src) {
	int i, j;
	for(i = 0, j = 0; src[i] != 0; i++, j++) {
		char c = src[i];
		if (c == '\\' || c == '\"')
			dest[j++] = '\\';
		dest[j] = c;
	}
	dest[j] = 0;
}

void logitem_append(int index, char* dateString) {
	if (s_log_item_count >= MAX_ACTIVITY_ITEMS || index >= s_active_item_count) { 
		LOG("Cannot append");
		return;
	}
	strcpy(s_log_items[s_log_item_count].date, dateString);
	safestrcpy(s_log_items[s_log_item_count].type, s_activity_items[index].type);
	safestrcpy(s_log_items[s_log_item_count].json, s_activity_items[index].json);
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
	store_keytoken();
	display_update_state();
	connect_update_state();
}

void set_sensorid(char *data) {
	strcpy(s_sensor_id, data);
	display_update_state();
	connect_update_state();
}
