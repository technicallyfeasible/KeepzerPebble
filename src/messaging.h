/*
 * Messaging stuff
 */


#ifndef __KEEPZER_MESSAGING__
#define __KEEPZER_MESSAGING__

#define MESSAGE_TYPE 0
#define MESSAGE_RESULT 1
#define MESSAGE_ITEM 10
#define MESSAGE_ITEM_NAME 11
#define MESSAGE_ITEM_COUNT 12
#define MESSAGE_SENSORID 20
#define MESSAGE_KEYTOKEN 21
#define MESSAGE_DATE 30
#define MESSAGE_DATATYPE 31
#define MESSAGE_JSON 32
#define MESSAGE_BATTERY 33
#define MESSAGE_TIMEZONE 34

void init_messaging();
void deinit_messaging();

void queue_item(int current_item);
void send_next_item();

/* get the account token of the current user */
void get_account_token();
/* cancel the current connection attempt */
void cancel_connect();
/* start a new connection attempt */
void connect();
/* send keytoken to configuration side so it's available for logging */
void sendKeyToken();
/* send sensor id to configuration side so it's available for logging */
void sendSensorId();
/* get timezone offset */
void getTimezoneOffset();

#endif