/*
 * Messaging stuff
 */


#ifndef __KEEPZER_MESSAGING__
#define __KEEPZER_MESSAGING__

#define MESSAGE_TYPE 0
#define MESSAGE_ITEM 10
#define MESSAGE_ITEM_NAME 11
#define MESSAGE_ITEM_COUNT 12
#define MESSAGE_ACCOUNTTOKEN 20
#define MESSAGE_MESSAGE 100

void init_messaging();

void send_log(char *text);
void script_log(char *format, int arg);
void script_log2(char *format, int arg1, int arg2);
void script_log3(char *format, int arg1, int arg2, int arg3);
void script_log4(char *format, int arg1, int arg2, int arg3, int arg4);

void send_item(int current_item);
void get_connection_state();

/* get the account token of the current user */
void get_account_token();

#endif