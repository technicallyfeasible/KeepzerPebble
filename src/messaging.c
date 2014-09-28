#include "pebble.h"
#include "messaging.h"
#include "storage.h"
#include "display.h"
#include "connect.h"

#define MAX_MESSAGES 4
#define MAX_VALUES 5
#define MAX_MESSAGE_SIZE 512

const uint32_t inbound_size = 256;
const uint32_t outbound_size = MAX_MESSAGE_SIZE;

static const char *message_type_log = "log";
static const char *message_type_log_result = "log_result";
static const char *message_type_item = "item";
static const char *message_type_sensorid = "sensorid";
static const char *message_type_keytoken = "keytoken";
static const char *message_type_timezone = "tz";
static const char *message_type_connect = "connect";
static const char *message_type_cancel_connect = "cancel_connect";

static bool logPending = false;

typedef struct {
	Tuplet data[MAX_VALUES];
	int data_count;
} Message;
static Message message_queue[MAX_MESSAGES];
static int message_queue_count = 0;
static bool messagePending = false;


/* remove the first message because it was sent */
static void remove_current_message() {
	if (message_queue_count <= 0)
		return;
	for (int i = 1; i < message_queue_count; i++)
		memcpy(&message_queue[i - 1], &message_queue[i], sizeof(Message));
	message_queue_count--;
	LOG("message removed");
}
/* send the first message in the queue */
static void send_current_message() {
	if (messagePending || message_queue_count <= 0)
		return;
	// if there is no connection then don't even try
	if (!bluetooth_connection_service_peek())
		return;

	messagePending = true;
	
	Message* message = &message_queue[0];
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	for (int i = 0; i < message->data_count; i++)
		dict_write_tuplet(iter, &message->data[i]);
	app_message_outbox_send();
	LOG("message sent");
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
	messagePending = false;
	remove_current_message();
	send_current_message();
}
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
	messagePending = false;
	logPending = false;
	
	LOG("send failed");
	Tuple *type_tuple = dict_find(failed, MESSAGE_TYPE);
	LOG(type_tuple->value->cstring);

	send_current_message();
}
static void in_received_handler(DictionaryIterator *iter, void *context) {
    // incoming message received
	// Check for fields you expect to receive
	Tuple *type_tuple = dict_find(iter, MESSAGE_TYPE);

	LOG(type_tuple->value->cstring);

	// Received items
    if (strcmp(type_tuple->value->cstring, message_type_item) == 0) {
		Tuple *count_tuple = dict_find(iter, MESSAGE_ITEM_COUNT);
		s_active_item_count = count_tuple->value->uint16;
		if (s_active_item_count > MAX_ACTIVITY_ITEMS)
			s_active_item_count = MAX_ACTIVITY_ITEMS;
		if (s_active_item_count > 0) {
			Tuple *item_tuple = dict_find(iter, MESSAGE_ITEM);
			Tuple *name_tuple = dict_find(iter, MESSAGE_ITEM_NAME);
			Tuple *type_tuple = dict_find(iter, MESSAGE_DATATYPE);
			Tuple *json_tuple = dict_find(iter, MESSAGE_JSON);
			int index = item_tuple->value->uint16;
			activity_set(index, name_tuple->value->cstring,  type_tuple->value->cstring, json_tuple->value->cstring);
			store_config(false, index);
		}
		else
			store_config(false, -1);
		display_update_events();
    }
    else if (strcmp(type_tuple->value->cstring, message_type_sensorid) == 0) {
		Tuple *token_tuple = dict_find(iter, MESSAGE_SENSORID);
		set_sensorid(token_tuple->value->cstring);
		LOG(token_tuple->value->cstring);
	}
    else if (strcmp(type_tuple->value->cstring, message_type_keytoken) == 0) {
		Tuple *token_tuple = dict_find(iter, MESSAGE_KEYTOKEN);
		set_keytoken(token_tuple->value->cstring);
		LOG(s_key_token);
		
		// we got a keytoken so we can send out pending events
		send_next_item();
	}
    else if (strcmp(type_tuple->value->cstring, message_type_timezone) == 0) {
		Tuple *token_tuple = dict_find(iter, MESSAGE_TIMEZONE);
		timezone = token_tuple->value->int32;
		store_timezone();
		//char tz[32];
		//snprintf(tz, 32, "%d", timezone);
		//LOG(tz);
	}
    else if (strcmp(type_tuple->value->cstring, message_type_log_result) == 0) {
		Tuple *token_tuple = dict_find(iter, MESSAGE_RESULT);
		int result = token_tuple->value->uint8;
		// if successful then remove the first log item and send the next one
		// result: 0=cannot send, 1=success, 2=connect error, 3=bad item
		logPending = false;
		if (result == 3 || result == 1)
			logitem_remove(0);
		if (result == 1 || result == 2 || result == 3)	// success or general error, try again
			send_next_item();
		if (result == 0) {				// unauthorized, clear token and force reauth
			LOG("Connect error");
			set_keytoken("\0");
		}
	}
}

static void connection_state_handler(bool connected) {
	if (!connected) return;
	
	send_next_item();
	send_current_message();
}

void init_messaging() {
	// Reduce the sniff interval for more responsive messaging at the expense of
	// increased energy consumption by the Bluetooth module
	// The sniff interval will be restored by the system after the app has been
	// unloaded
	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);

	app_message_register_inbox_received(in_received_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);

	app_message_open(inbound_size, outbound_size);
	
	// subscribe to connection state changes
	bluetooth_connection_service_subscribe(connection_state_handler);
}
void deinit_messaging() {
	app_message_deregister_callbacks();
	bluetooth_connection_service_unsubscribe();
}

/* queue the next message to be sent */
static void queue_message(Message **message) {
	// just drop the message if the queue is full
	if (message_queue_count >= MAX_MESSAGES)
		return;
	*message = &message_queue[message_queue_count];
	message_queue[message_queue_count].data_count = 0;
	message_queue_count++;
	LOG("message queued");
}
/* add string data to the message */
static void message_add_cstring(Message *message, int key, const char* data) {
	Tuplet token = ((Tuplet) { .type = TUPLE_CSTRING, .key = key, .cstring = { .data = data, .length = data ? strlen(data) + 1 : 0 }});
	memcpy(&message->data[message->data_count], &token, sizeof(Tuplet));
	message->data_count++;
}
/* add int data to the message */
static void message_add_int(Message *message, int key, const int data) {
	Tuplet token = ((const Tuplet) { .type = TUPLE_INT, .key = key, .integer = { .storage = data, .width = sizeof(data) }});
	memcpy(&message->data[message->data_count], &token, sizeof(Tuplet));
	message->data_count++;
}

void queue_item(int current_item) {
	// append the item to the list of sending items
	time_t date;
	time(&date);
	struct tm *localDate = localtime(&date);
	char utcDate[MAX_ITEM_DATE_LENGTH];
	strftime(utcDate, MAX_ITEM_DATE_LENGTH, "%FT%T", localDate);
	char formattedDate[MAX_ITEM_DATE_LENGTH];
	char sign = '+';
	if (timezone < 0){
		sign = '-';
		timezone = -timezone;
	}
	snprintf(formattedDate, MAX_ITEM_DATE_LENGTH, "%s%c%02d:%02d", utcDate, sign, timezone/60, timezone%60);
	LOG(formattedDate);
	logitem_append(current_item, formattedDate);
	
	send_next_item();
}
void send_next_item() {
	if (s_log_item_count == 0 || logPending || strlen(s_key_token) == 0)
		return;
	logPending = true;
	
	LogItem *item = &s_log_items[0];

	Message *message = NULL;
	queue_message(&message);
	if (message == NULL) {
		logPending = false;
		return;
	}
	message_add_cstring(message, MESSAGE_TYPE, message_type_log);
	message_add_cstring(message, MESSAGE_DATE, item->date);
	message_add_cstring(message, MESSAGE_DATATYPE, item->type);
	message_add_cstring(message, MESSAGE_JSON, item->json);
	if (item->battery <= 200)
		message_add_int(message, MESSAGE_BATTERY, item->battery);
	send_current_message();
}

/* cancel the current connection attempt */
void cancel_connect() {
	Message *message = NULL;
	queue_message(&message);
	if (message == NULL) return;
	message_add_cstring(message, MESSAGE_TYPE, message_type_cancel_connect);
	send_current_message();
}
/* start a new connection attempt */
void connect() {
	Message *message = NULL;
	queue_message(&message);
	if (message == NULL) return;
	message_add_cstring(message, MESSAGE_TYPE, message_type_connect);
	send_current_message();
}

/* send keytoken to configuration side so it's available for logging */
void sendKeyToken() {
	// don't send keytoken if it is empty
	if (strlen(s_key_token) == 0)
		return;
	Message *message = NULL;
	queue_message(&message);
	if (message == NULL) return;
	message_add_cstring(message, MESSAGE_TYPE, message_type_keytoken);
	message_add_cstring(message, MESSAGE_KEYTOKEN, s_key_token);
	send_current_message();
}
/* send sensor id to configuration side so it's available for logging */
void sendSensorId() {
	Message *message = NULL;
	queue_message(&message);
	if (message == NULL) return;
	message_add_cstring(message, MESSAGE_TYPE, message_type_sensorid);
	message_add_cstring(message, MESSAGE_SENSORID, s_sensor_id);
	send_current_message();
}
