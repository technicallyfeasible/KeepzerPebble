#include "pebble.h"
#include "settings.h"

static void settings_click_handler(ClickRecognizerRef recognizer, Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Navigate Settings");

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
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) settings_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) settings_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) settings_click_handler);
}

/* initialize the settings */
void init_settings(Window *window) {
}

/* deinitialize the settings */
void deinit_settings(Window *window) {
}
