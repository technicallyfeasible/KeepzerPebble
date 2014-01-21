#include "pebble.h"
#include "mini-printf.h"
#include "messaging.h"
#include "storage.h"
#include "display.h"

const uint32_t inbound_size = 256;
const uint32_t outbound_size = 512;

static const char *message_type_log = "log";
static const char *message_type_log_result = "log_result";
static const char *message_type_item = "item";
static const char *message_type_sensorid = "sensorid";
static const char *message_type_keytoken = "keytoken";
static const char *message_type_account_token = "account_token";
static const char *message_type_connect = "connect";
static const char *message_type_cancel_connect = "cancel_connect";

bool logPending = false;

static void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
}
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending message failed. Trying again.");
	Tuple *type_tuple = dict_find(failed, MESSAGE_TYPE);
	APP_LOG(APP_LOG_LEVEL_DEBUG, type_tuple->value->cstring);

	if (strcmp(type_tuple->value->cstring, message_type_keytoken) == 0)
		sendKeyToken();
	else if (strcmp(type_tuple->value->cstring, message_type_log) == 0) {
		logPending = false;
		send_next_item();
	}
}
static void in_received_handler(DictionaryIterator *iter, void *context) {
    // incoming message received
	// Check for fields you expect to receive
	Tuple *type_tuple = dict_find(iter, MESSAGE_TYPE);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Received app message");
	APP_LOG(APP_LOG_LEVEL_DEBUG, type_tuple->value->cstring);

	// Received items
    if (strcmp(type_tuple->value->cstring, message_type_item) == 0) {
		Tuple *item_tuple = dict_find(iter, MESSAGE_ITEM);
		Tuple *name_tuple = dict_find(iter, MESSAGE_ITEM_NAME);
		Tuple *type_tuple = dict_find(iter, MESSAGE_DATATYPE);
		Tuple *json_tuple = dict_find(iter, MESSAGE_JSON);
		Tuple *count_tuple = dict_find(iter, MESSAGE_ITEM_COUNT);
		s_active_item_count = count_tuple->value->uint16;
		char *name = name_tuple->value->cstring;
		char *type = type_tuple->value->cstring;
		char *json = json_tuple->value->cstring;
		activity_set(item_tuple->value->uint16, name, type, json);
		store_config();
		display_update_events();
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, name);
		APP_LOG(APP_LOG_LEVEL_DEBUG, type);
		APP_LOG(APP_LOG_LEVEL_DEBUG, json);
    }
    else if (strcmp(type_tuple->value->cstring, message_type_sensorid) == 0) {
		Tuple *token_tuple = dict_find(iter, MESSAGE_SENSORID);
		APP_LOG(APP_LOG_LEVEL_DEBUG, token_tuple->value->cstring);
		set_sensorid(token_tuple->value->cstring);
		display_update_state();
	}
    else if (strcmp(type_tuple->value->cstring, message_type_keytoken) == 0) {
		Tuple *token_tuple = dict_find(iter, MESSAGE_KEYTOKEN);
		set_keytoken(token_tuple->value->cstring);
		APP_LOG(APP_LOG_LEVEL_DEBUG, s_key_token);
		store_keytoken();
		display_update_state();
		
		// we got a keytoken so we can send out pending events
		send_next_item();
	}
    else if (strcmp(type_tuple->value->cstring, message_type_log_result) == 0) {
		Tuple *token_tuple = dict_find(iter, MESSAGE_RESULT);
		int result = token_tuple->value->uint8;
		//APP_LOG(APP_LOG_LEVEL_DEBUG, result);
		// if successful then remove the first log item and send the next one
		if (result == 1)
			logitem_remove(0);
		logPending = false;
		send_next_item();
	}
}
static void in_dropped_handler(AppMessageResult reason, void *context) {
	// incoming message dropped
}


void init_messaging() {
	// Reduce the sniff interval for more responsive messaging at the expense of
	// increased energy consumption by the Bluetooth module
	// The sniff interval will be restored by the system after the app has been
	// unloaded
	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);

	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);

	app_message_open(inbound_size, outbound_size);
}


void queue_item(int current_item) {
	// append the item to the list of sending items
	time_t date;
	time(&date);
	struct tm *localDate = localtime(&date);
	char formattedDate[MAX_ITEM_DATE_LENGTH];
	strftime(formattedDate, MAX_ITEM_DATE_LENGTH, "%FT%TZ", localDate);
	APP_LOG(APP_LOG_LEVEL_DEBUG, formattedDate);
	logitem_append(current_item, formattedDate);
	
	send_next_item();
}
void send_next_item() {
	if (s_log_item_count == 0 || logPending || strlen(s_key_token) == 0)
		return;
	logPending = true;
	
	LogItem *item = &s_log_items[0];

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending item.");

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_log);
	dict_write_int16(iter, MESSAGE_ITEM, current_item);
	dict_write_cstring(iter, MESSAGE_DATE, item->date);
	dict_write_cstring(iter, MESSAGE_DATATYPE, item->type);
	dict_write_cstring(iter, MESSAGE_JSON, item->json);
	app_message_outbox_send();
}

/* get the account token of the current user */
void get_account_token() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_account_token);
	app_message_outbox_send();
}

/* cancel the current connection attempt */
void cancel_connect() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_cancel_connect);
	app_message_outbox_send();
}
/* start a new connection attempt */
void connect() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_connect);
	app_message_outbox_send();
}

/* send keytoken to configuration side so it's available for logging */
void sendKeyToken() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_keytoken);
	dict_write_cstring(iter, MESSAGE_KEYTOKEN, s_key_token);
	app_message_outbox_send();
}
