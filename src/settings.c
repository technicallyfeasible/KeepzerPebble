#include "pebble.h"
#include "display.h"
#include "settings.h"
#include "storage.h"
#include "messaging.h"

static Window *settings_window = NULL;
static SimpleMenuLayer *menu_layer = NULL;

//#define MENU_HELP 0
#define MENU_DISCONNECT 0
static SimpleMenuItem menu_items_root[2];
static SimpleMenuSection sections_root[1];

static void menu_select_callback(int index, void *ctx) {
	switch(index) {
		//case MENU_HELP:
		//	break;
		case MENU_DISCONNECT:
			set_keytoken("\0");
			sendKeyToken();
			// leave the options menu
			window_stack_pop(true);
			break;
	}
}


static void create_menu(Window *window) {
	/*menu_items_root[MENU_HELP] = (SimpleMenuItem) {
		.title = "Help",
		.callback = menu_select_callback,
		.icon = icon_info
	};*/
	menu_items_root[MENU_DISCONNECT] = (SimpleMenuItem) {
		.title = "Disconnect",
		.callback = menu_select_callback,
		.icon = icon_disconnect
	};
	sections_root[0] = (SimpleMenuSection) {
		//.title = "Options",
		.num_items = 1,
		.items = menu_items_root,
	};
	
	menu_layer = simple_menu_layer_create(bounds, window, sections_root, 1, NULL);
	layer_add_child(window_get_root_layer(window), simple_menu_layer_get_layer(menu_layer));
}

/* initialize the settings */
static void init_settings(Window *window) {
	create_menu(window);
}

/* deinitialize the settings */
static void deinit_settings(Window *window) {
	simple_menu_layer_destroy(menu_layer);
}

/* start the settings window */
void settings_start() {
	if (settings_window == NULL) {
		settings_window = window_create();
		//window_set_fullscreen(settings_window, true);
		window_set_window_handlers(settings_window, (WindowHandlers) {
			.load = init_settings,
			.unload = deinit_settings
		});
	}
	window_stack_push(settings_window, true);
}
/* destroy settings resource */
void settings_destroy() {
	if (settings_window == NULL)
		return;
	window_destroy(settings_window);
}
