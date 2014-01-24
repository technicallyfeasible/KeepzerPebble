#include "pebble.h"
#include "display.h"
#include "settings.h"

static Layer *step1_layer = NULL;
static Layer *step2_layer = NULL;
static Layer *step3_layer = NULL;
static Layer *step4_layer = NULL;

static int step = 0;

static char *text_connect_step1 = "Connect your phone to the Internet.";


static void create_step1(Window *window) {
	step1_layer = layer_create(bounds);
	layer_add_child(window_get_root_layer(window), step1_layer);
	
	TextLayer *text_layer = text_layer_create(GRect(0, 0, bounds.size.w - 8, bounds.size.h));
	text_layer_set_background_color(text_layer, GColorClear);
	text_layer_set_font(text_layer, titleFont);
	text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
	text_layer_set_text(text_layer, text_connect_step1);
	layer_add_child(step1_layer, text_layer_get_layer(text_layer));
	
}

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
	step = 0;
	create_step1(window);
}

/* deinitialize the settings */
void deinit_settings(Window *window) {
}
