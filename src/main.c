#include "pebble.h"
#include "mini-printf.h"
#include "messaging.h"
#include "storage.h"
#include "display.h"

#include <stdlib.h>
#include <string.h>

	
/* Constants */
static char* empty_text = "";
static char* connect_text = "Press CONNECT to connect to your Keepzer account";
static char* connecting_text = "Enter the id shown below in your Keepzer account to confirm the link";
static char* connected_text = "CONNECTED and ready for logging";
static char* notification_text_connected = "connected";
static char* notification_text_disconnected = "not connected";
static char* notification_text_connecting = "connecting...";
static char* notification_text_pending = "%d logs pending";

/* General window stuff */
static Window *window;
static GRect bounds;

/* Layers*/
static BitmapLayer *logo_layer = NULL;
static Layer *navi_layer = NULL;
static TextLayer *confirm_text_layer = NULL;
static Layer *events_layer = NULL;
static PropertyAnimation *prop_animation;

static Layer *state_layer = NULL;
static TextLayer *state_text_layer_top = NULL;
static TextLayer *state_text_layer_bottom = NULL;
static PropertyAnimation *state_layer_animation;

static TextLayer *notification_text_layer = NULL;

static GBitmap *arrow_up_image;
static GBitmap *arrow_down_image;
static GBitmap *logo_image;

static int state = 0;			// 0: disconnected, 1: connecting, 2: connected
static int screen_count = 1;

static char notification_buffer[128] = "\0";



static void select_current_item() {
	// state screen
	if (current_item <= 0) {
		switch(state) {
			/* disconnected */
			case 0:
				connect();
				state = 1;
				break;
			/* connecting */
			case 1:
				s_key_token[0] = 0;
				state = 0;
				store_keytoken();
				cancel_connect();
				sendKeyToken();
				break;
			/* connected */
			case 2:
				s_key_token[0] = 0;
				state = 0;
				store_keytoken();
				sendKeyToken();
				break;
		}
		layer_mark_dirty(state_layer);
	}
	else
		queue_item(current_item - 1);
}

static void destroy_property_animation(PropertyAnimation **animation) {
	if (*animation == NULL) {
		return;
	}
	if (animation_is_scheduled((Animation*) *animation)) {
		animation_unschedule((Animation*) *animation);
	}
	property_animation_destroy(*animation);
	*animation = NULL;
}

static void show_state(int from, int to) {
	int height = bounds.size.h;
	destroy_property_animation(&state_layer_animation);

	GRect from_rect = GRect(0, from * bounds.size.h, bounds.size.w, height);
	GRect to_rect = GRect(0, to * bounds.size.h, bounds.size.w, height);
	state_layer_animation = property_animation_create_layer_frame(state_layer, &from_rect, &to_rect);
	animation_set_duration((Animation*) state_layer_animation, 400);
	animation_schedule((Animation*) state_layer_animation);
}

static void show_event(int index) {
	int screen_count = s_active_item_count + 1;
	
	int height = bounds.size.h * (screen_count + 2);
	destroy_property_animation(&prop_animation);

	bool invert = false;
	GRect from_rect = GRect(0, -(current_item + 1) * bounds.size.h, bounds.size.w, height);
	if(index < 0) {
		index = screen_count - 1;
		from_rect.origin.y = -(screen_count + 1) * bounds.size.h;
		invert = true;
	}
	else if(index >= screen_count) {
		index = 0;
		from_rect.origin.y = 0;
		invert = true;
	}
	GRect to_rect = GRect(0, -(index + 1) * bounds.size.h, bounds.size.w, height);
	prop_animation = property_animation_create_layer_frame(events_layer, &from_rect, &to_rect);
	animation_set_duration((Animation*) prop_animation, 400);
	animation_schedule((Animation*) prop_animation);

	// if state is about to be shown or hidden then animate it
	if (index == 0 || current_item == 0) {
		int stateFrom = 0, stateTo = 0;
		if (current_item == 1)
			stateFrom = -1;
		else if (current_item > 1)
			stateFrom = 1;
		if (index == 1)
			stateTo = -1;
		else if (index > 1)
			stateTo = 1;
		if (invert && screen_count == 2) {
			stateFrom = -stateFrom;
			stateTo = -stateTo;
		}
		show_state(stateFrom, stateTo);
	}
	
	current_item = index;
	layer_mark_dirty(navi_layer);
}

static void click_handler(ClickRecognizerRef recognizer, Window *window) {
	int screen_count = s_active_item_count + 1;
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Navigate");

	int next_item = current_item;
	switch (click_recognizer_get_button_id(recognizer)) {
		case BUTTON_ID_UP:
			if (screen_count <= 1)
				break;
			next_item = current_item - 1;
			show_event(next_item);
		break;

		case BUTTON_ID_DOWN:
			if (screen_count <= 1)
				break;
			next_item = current_item + 1;
			show_event(next_item);
		break;

		default:
		case BUTTON_ID_SELECT:
			select_current_item();
		break;
	}
}

static void load_resources() {
	arrow_up_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW_UP);
	arrow_down_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW_DOWN);
	logo_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);
}

static void set_notification(char* text) {
	if (notification_text_layer == NULL)
		return;
	strcpy(notification_buffer, text);
	text_layer_set_text(notification_text_layer, notification_buffer);
}
static void create_logo_layer(Window *window) {
	GRect logoBounds = logo_image->bounds;

	// just to cancel out the background
	TextLayer *text_layer = text_layer_create(GRect(0, bounds.size.h - logoBounds.size.h, bounds.size.w, logoBounds.size.h));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
	
	// display logo
	logo_layer = bitmap_layer_create((GRect) { .origin = { 3, bounds.size.h - logoBounds.size.h }, .size = logoBounds.size });
	bitmap_layer_set_bitmap(logo_layer, logo_image);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(logo_layer));

	GFont statusFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);

	// display notofications
	int offset = 4;
	notification_text_layer = text_layer_create(GRect(logoBounds.size.w + 7, bounds.size.h - logoBounds.size.h + offset, bounds.size.w - logoBounds.size.w - 7, logoBounds.size.h - offset));
	text_layer_set_background_color(notification_text_layer, GColorClear);
	text_layer_set_font(notification_text_layer, statusFont);
	text_layer_set_text_alignment(notification_text_layer, GTextAlignmentLeft);
	text_layer_set_overflow_mode(notification_text_layer, GTextOverflowModeTrailingEllipsis);
	//text_layer_set_text(notification_text_layer, "noti");
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(notification_text_layer));
}

static void navi_layer_update_callback(Layer *me, GContext* ctx) {
	int screen_count = s_active_item_count + 1;
	
	if(screen_count > 1) {
		GRect arrowBounds = arrow_up_image->bounds;
		graphics_draw_bitmap_in_rect(ctx, arrow_up_image, (GRect) { .origin = { bounds.size.w - arrowBounds.size.w, 0 }, .size = arrowBounds.size });
		arrowBounds = arrow_down_image->bounds;
		graphics_draw_bitmap_in_rect(ctx, arrow_down_image, (GRect) { .origin = { bounds.size.w - arrowBounds.size.w, bounds.size.h - arrowBounds.size.h }, .size = arrowBounds.size });

		int length = bounds.size.h / screen_count;
		int y = bounds.size.h * current_item / screen_count;
		graphics_draw_rect(ctx, GRect(bounds.size.w - 2, y, 2, length));
	}
	if(current_item == 0) {
		switch(state) {
			/* disconnected */
			case 0:
				text_layer_set_text(confirm_text_layer, "CONNECT");
				break;
			/* connecting */
			case 1:
				text_layer_set_text(confirm_text_layer, "CANCEL");
				break;
			/* connected */
			case 2:
				text_layer_set_text(confirm_text_layer, "DISCONNECT");
				break;
		}
	}
	else
		text_layer_set_text(confirm_text_layer, "LOG");
}
static void create_navi(Window *window) {
	navi_layer = layer_create(bounds);
	layer_set_update_proc(navi_layer, navi_layer_update_callback);
	layer_add_child(window_get_root_layer(window), navi_layer);

	// create action text layer
	confirm_text_layer = text_layer_create(GRect(0, (bounds.size.h - 16)/2, bounds.size.w - 4, 16));
	text_layer_set_background_color(confirm_text_layer, GColorClear);
	layer_add_child(navi_layer, text_layer_get_layer(confirm_text_layer));
	text_layer_set_text_alignment(confirm_text_layer, GTextAlignmentRight);
	text_layer_set_text(confirm_text_layer, "LOG");
}

static void state_layer_update_callback(Layer *me, GContext* ctx) {
	if (state_text_layer_top == NULL || state_text_layer_bottom == NULL)
		return;
	
	char *topText = NULL, *bottomText = NULL;
	switch(state) {
		/* disconnected */
		case 0:
			topText = connect_text;
			bottomText = empty_text;
			break;
		/* connecting */
		case 1:
			topText = connecting_text;
			bottomText = s_sensor_id;
			break;
		/* connected */
		case 2:
			topText = connected_text;
			bottomText = empty_text;
			break;
	}
	text_layer_set_text(state_text_layer_top, topText);
	text_layer_set_text(state_text_layer_bottom, bottomText);
}
static void create_state_layer(Window *window) {
	int posIndex = 0;
	if (current_item != 0)
		posIndex = 1;
	state_layer = layer_create(GRect(0, posIndex * bounds.size.h, bounds.size.w, bounds.size.h));
	layer_set_update_proc(state_layer, state_layer_update_callback);
	layer_add_child(window_get_root_layer(window), state_layer);

	
	GFont bigFont = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
	GFont smallFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);

	TextLayer *text_layer = text_layer_create(GRect(2,0, bounds.size.w, bounds.size.h));
	text_layer_set_background_color(text_layer, GColorClear);
	text_layer_set_font(text_layer, bigFont);
	text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
	text_layer_set_text(text_layer, "Keepzer for Pebble");
	layer_add_child(state_layer, text_layer_get_layer(text_layer));

	state_text_layer_top = text_layer_create(GRect(2, 24, bounds.size.w, (bounds.size.h / 2) - 24));
	text_layer_set_background_color(state_text_layer_top, GColorClear);
	text_layer_set_font(state_text_layer_top, smallFont);
	text_layer_set_text_alignment(state_text_layer_top, GTextAlignmentLeft);
	//text_layer_set_text(state_text_layer_top, "Press CONNECT to connect to your Keepzer account");
	layer_add_child(state_layer, text_layer_get_layer(state_text_layer_top));

	state_text_layer_bottom = text_layer_create(GRect(0, bounds.size.h / 2 + 12, bounds.size.w, (bounds.size.h / 2) - 40));
	text_layer_set_background_color(state_text_layer_bottom, GColorClear);
	text_layer_set_font(state_text_layer_bottom, bigFont);
	text_layer_set_text_alignment(state_text_layer_bottom, GTextAlignmentLeft);
	//text_layer_set_text(state_text_layer_bottom, "DThzEzhoIx2GvRtj1");
	layer_add_child(state_layer, text_layer_get_layer(state_text_layer_bottom));
}
static void add_event_layer(GFont font, int itemIndex, int posIndex) {
	TextLayer *text_layer = text_layer_create(GRect(2, posIndex * bounds.size.h, bounds.size.w - 26, bounds.size.h));
	text_layer_set_background_color(text_layer, GColorClear);
	text_layer_set_font(text_layer, font);
	text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
	text_layer_set_text(text_layer, s_activity_items[itemIndex].name);
	layer_add_child(events_layer, text_layer_get_layer(text_layer));
}
static void create_events(Window *window) {
	int screen_count = s_active_item_count + 1;
	
	if (events_layer != NULL)
		layer_destroy(events_layer);

	events_layer = layer_create(GRect(0, -(current_item + 1) * bounds.size.h, bounds.size.w, bounds.size.h * (screen_count + 2)));
	layer_add_child(window_get_root_layer(window), events_layer);

	GFont font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
	
	if(s_active_item_count > 0) {
		int offset = screen_count - s_active_item_count;
		int i;
		add_event_layer(font, s_active_item_count - 1, 0);
		//add_state_layer(1);
		for (i = 0; i < s_active_item_count; i++) {
			add_event_layer(font, i, i + 1 + offset);
		}
		//add_state_layer(screen_count + 1);
		//add_event_layer(font, 0, s_active_item_count + 1 + offset);
	}
	//else
	//	add_state_layer(1);
}

static void config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) click_handler);
}

static void init(Window *window) {
	/*window = window_create();
	window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
	window_set_fullscreen(window, true);
	window_stack_push(window, false);*/

	Layer *window_layer = window_get_root_layer(window);
	// Get the bounds of the window for sizing the text layer
	bounds = layer_get_bounds(window_layer);

	init_messaging();
	load_keytoken();
	load_config();
	load_log();

	load_resources();

	// check if we have a keytoken, if not then show the connection screen
	if (strlen(s_key_token) == 0)
		state = 0;
	else {
		state = 2;
		sendKeyToken();
	}
	// send pending items
	//send_next_item();
	if (s_active_item_count == 0)
		activity_append("log me", "keepzer.calendar.event", "{\"event\":\"log me\"}");
		
	create_logo_layer(window);
	create_events(window);
	create_state_layer(window);
	create_navi(window);
	
	display_update_state();
}

static void deinit(Window *window) {
	store_config();
	destroy_property_animation(&prop_animation);
	destroy_property_animation(&state_layer_animation);

	layer_destroy(navi_layer);
	layer_destroy(events_layer);
}

int main(void) {
	window = window_create();
	window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
	window_set_fullscreen(window, true);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = init,
		.unload = deinit
	});
	window_stack_push(window, false);

	//init();
	app_event_loop();
	//deinit();
	window_destroy(window);
}


void display_update_state() {
	// if state indicates we are connected but there is no token then set state back to disconnected
	if (strlen(s_key_token) == 0 && state > 1) {
		state = 0;
		layer_mark_dirty(state_layer);
	}
	// if state indicates we are disconnected but there is a token then set state to connected
	else if (strlen(s_key_token) > 0 && state != 2) {
		state = 2;
		layer_mark_dirty(state_layer);
	}
	else if(state == 1)
		layer_mark_dirty(state_layer);

	/* Update notification bar */
	if(s_log_item_count == 0)
		set_notification(empty_text);
	else if(s_log_item_count == 1)
		set_notification("1 event pending");
	else {
		char text[32];
		mini_snprintf(text, 32, "%d events pending", s_log_item_count);
		set_notification(text);
	}
}

void display_update_events() {
	create_events(window);
}


