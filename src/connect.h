/*
 * Connection stuff
 */


#ifndef __KEEPZER_CONNECT__
#define __KEEPZER_CONNECT__

/* start the connection process */
void connect_start();
/* destroy connect process resources */
void connect_destroy();

/* update the connection state */
void connect_update_state();

#endif
