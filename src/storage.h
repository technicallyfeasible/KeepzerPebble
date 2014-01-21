/*
 * Storage stuff
 */


#ifndef __KEEPZER_STORAGE__
#define __KEEPZER_STORAGE__

#define MAX_ACTIVITY_ITEMS (20)
#define MAX_ITEM_TEXT_LENGTH (32)
#define MAX_ITEM_DATE_LENGTH (24)
#define MAX_ITEM_TYPE_LENGTH (64)
#define MAX_ITEM_JSON_LENGTH (128)
#define MAX_LOG_ITEMS (10)

#define STORAGE_ITEM_COUNT 0
#define STORAGE_ITEMS 1
#define STORAGE_ITEM_CURRENT 2
#define STORAGE_KEYTOKEN 3
#define STORAGE_LOG_COUNT 4
#define STORAGE_LOGS 5

extern char s_key_token[128];
extern char s_sensor_id[64];
typedef struct {
  char name[MAX_ITEM_TEXT_LENGTH];
  char type[MAX_ITEM_TYPE_LENGTH];
  char json[MAX_ITEM_JSON_LENGTH];
} ActivityItem;
extern ActivityItem s_activity_items[MAX_ACTIVITY_ITEMS];
extern int s_active_item_count;
extern int current_item;

/* queue for items being logged */
typedef struct {
  char date[MAX_ITEM_DATE_LENGTH];
  char type[MAX_ITEM_TYPE_LENGTH];
  char json[MAX_ITEM_JSON_LENGTH];
} LogItem;
extern LogItem s_log_items[MAX_LOG_ITEMS];
extern int s_log_item_count;

/* Store all activities in persistent storage */
void store_config();
/* Load all activities in persistent storage */
void load_config();
/* Store the keytoken */
void store_keytoken();
/* Load the keytoken */
void load_keytoken();
/* Load all log items from persistent storage */
void load_log();

void activity_append(char *name, char* type, char* json);
void activity_set(int index, char *name, char* type, char* json);
void logitem_append(int index, char* dateString);
void logitem_remove(int index);

void set_keytoken(char *data);
void set_sensorid(char *data);

#endif