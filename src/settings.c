#include "pebble.h"
#include "display.h"
#include "settings.h"
#include "storage.h"
#include "messaging.h"

static Window *settings_window = NULL;
static SimpleMenuLayer *menu_layer = NULL;

#define MENU_HELP 0
#define MENU_DISCONNECT 1
static SimpleMenuItem menu_items_root[2];
static SimpleMenuSection sections_root[1];

static void menu_select_callback(int index, void *ctx) {
	switch(index) {
		case MENU_HELP:
			break;
		case MENU_DISCONNECT:
			s_key_token[0] = 0;
			store_keytoken();
			sendKeyToken();
			display_update_state();
			// leave the options menu
			window_stack_pop(true);
			break;
	}
}


static void create_menu(Window *window) {
	menu_items_root[MENU_HELP] = (SimpleMenuItem) {
		.title = "Help",
		.callback = menu_select_callback,
		.icon = icon_info
	};
	menu_items_root[MENU_DISCONNECT] = (SimpleMenuItem) {
		.title = "Disconnect",
		.callback = menu_select_callback,
		.icon = icon_disconnect
	};
	sections_root[0] = (SimpleMenuSection) {
		.title = "Options",
		.num_items = 2,
		.items = menu_items_root,
	};
	
	menu_layer = simple_menu_layer_create(bounds, window, sections_root, 1, NULL);
	layer_add_child(window_get_root_layer(window), simple_menu_layer_get_layer(menu_layer));
}

static void settings_click_handler(ClickRecognizerRef recognizer, Window *window) {
	switch (click_recognizer_get_button_id(recognizer)) {
		case BUTTON_ID_UP:
			break;
		case BUTTON_ID_DOWN:
			break;
		default:
		case BUTTON_ID_SELECT:
			break;
	}
}

/* configure click handlers */
void settings_config_provider(Window *window) {
	/*window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) settings_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) settings_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) settings_click_handler);*/
}

/* initialize the settings */
void init_settings(Window *window) {
	create_menu(window);
}

/* deinitialize the settings */
void deinit_settings(Window *window) {
	simple_menu_layer_destroy(menu_layer);
}

/* start the settings window */
void settings_start() {
	if (settings_window == NULL) {
		settings_window = window_create();
		window_set_click_config_provider(settings_window, (ClickConfigProvider) settings_config_provider);
		window_set_fullscreen(settings_window, true);
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

/* update the connection state */
void settings_update_state() {
}
