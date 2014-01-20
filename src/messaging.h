/*
 * Messaging stuff
 */


#ifndef __KEEPZER_MESSAGING__
#define __KEEPZER_MESSAGING__

#define MESSAGE_TYPE 0
#define MESSAGE_ITEM 10
#define MESSAGE_ITEM_NAME 11
#define MESSAGE_ITEM_COUNT 12
#define MESSAGE_SENSORID 20
#define MESSAGE_KEYTOKEN 21
#define MESSAGE_MESSAGE 100

void init_messaging();

void send_item(int current_item);

/* get the account token of the current user */
void get_account_token();
/* cancel the current connection attempt */
void cancel_connect();
/* start a new connection attempt */
void connect();
/* send keytoken to configuration side so it's available for logging */
void sendKeyToken();

#endif