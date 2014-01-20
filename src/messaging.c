#include "pebble.h"
#include "mini-printf.h"
#include "messaging.h"
#include "storage.h"
#include "display.h"

const uint32_t inbound_size = 128;
const uint32_t outbound_size = 128;

static const char *message_type_log = "log";
static const char *message_type_item = "item";
static const char *message_type_state = "state";
static const char *message_type_message = "message";
static const char *message_type_keytoken = "keytoken";
static const char *message_type_account_token = "account_token";
static const char *message_type_connect = "connect";


static void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
}
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
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
		Tuple *count_tuple = dict_find(iter, MESSAGE_ITEM_COUNT);
		s_active_item_count = count_tuple->value->uint16;
		activity_set(item_tuple->value->uint16, name_tuple->value->cstring);
		display_update_events();
		store_config();
    }
    else if (strcmp(type_tuple->value->cstring, message_type_keytoken) == 0) {
		Tuple *token_tuple = dict_find(iter, MESSAGE_KEYTOKEN);
		APP_LOG(APP_LOG_LEVEL_DEBUG, token_tuple->value->cstring);
		set_keytoken(token_tuple->value->cstring);
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


void send_log(char *text) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_message);
	dict_write_cstring(iter, MESSAGE_MESSAGE, text);
	app_message_outbox_send();
}
void script_log(char *format, int arg) {
	char text[128];
	mini_snprintf(text, 128, format, arg);
	send_log(text);
}
void script_log2(char *format, int arg1, int arg2) {
	char text[160];
	mini_snprintf(text, 160, format, arg1, arg2);
	send_log(text);
}
void script_log3(char *format, int arg1, int arg2, int arg3) {
	char text[192];
	mini_snprintf(text, 192, format, arg1, arg2, arg3);
	send_log(text);
}
void script_log4(char *format, int arg1, int arg2, int arg3, int arg4) {
	char text[256];
	mini_snprintf(text, 256, format, arg1, arg2, arg3, arg4);
	send_log(text);
}

void send_item(int current_item) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_log);
	dict_write_int16(iter, MESSAGE_ITEM, current_item);
	app_message_outbox_send();
}

void get_connection_state() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_state);
	app_message_outbox_send();
}

/* get the account token of the current user */
void get_account_token() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_account_token);
	app_message_outbox_send();
}

/* start a new connection attempt */
void connect() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_connect);
	app_message_outbox_send();
}
