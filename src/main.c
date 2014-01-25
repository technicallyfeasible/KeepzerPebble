#include "pebble.h"
#include "messaging.h"
#include "storage.h"
#include "display.h"
#include "connect.h"
#include "settings.h"

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
static char* notification_text_pending = "%d events pending";
static char* notification_text_pending_one = "1 event pending";
static char* navigation_text_connect = "CONNECT";
static char* navigation_text_disconnect = "DISCONNECT";
static char* navigation_text_cancel = "CANCEL";
static char* navigation_text_log = "5";
static char* navigation_text_start = "3";
static char* navigation_text_options = "";
static char* text_start_title = "Connect\nto\nKeepzer";
static char* text_start_subtitle = "(You must connect to make full use of the product)";
static char* text_start_options = "Options";

GFont symbolFont, titleFont, subtitleFont, eventsFont, statusFont, smallFont, tinyFont;

/* General window stuff */
static Window *window;
GRect bounds;

/* Layers*/
static BitmapLayer *logo_layer = NULL;
static Layer *notification_layer = NULL;
static TextLayer *notification_text_layer = NULL;

static Layer *navi_layer = NULL;
static TextLayer *confirm_text_layer = NULL;
static Layer *events_layer = NULL;
static PropertyAnimation *prop_animation;

static Layer *state_layer = NULL;
static TextLayer *state_text_layer_top = NULL;
static TextLayer *state_text_layer_bottom = NULL;
static PropertyAnimation *state_layer_animation;

GBitmap *arrow_up_image, *arrow_down_image, *logo_image, *icon_info, *icon_disconnect;

static int state = 0, lastState = -1;			// 0: disconnected, 1: connecting, 2: connected
static int screen_count = 1;

static char notification_buffer[128] = "\0";



static void select_current_item() {
	// state screen
	if (current_item <= 0) {
		if(state == 2) {
			// if connected then go to options
			settings_start();
		} else {
			// start connection process
			connect_start();
		}
	}
	else
		queue_item(current_item - 1);
}

void destroy_property_animation(PropertyAnimation **animation) {
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
	
	LOG("Navigate");

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
	icon_info = gbitmap_create_with_resource(RESOURCE_ID_ICON_INFO);
	icon_disconnect = gbitmap_create_with_resource(RESOURCE_ID_ICON_DISCONNECT);
}
static void unload_resources() {
	gbitmap_destroy(arrow_up_image);
	gbitmap_destroy(arrow_down_image);
	gbitmap_destroy(logo_image);
	gbitmap_destroy(icon_info);
	gbitmap_destroy(icon_disconnect);
}

static void set_notification(char* text) {
	if (notification_text_layer == NULL)
		return;
	strcpy(notification_buffer, text);
	text_layer_set_text(notification_text_layer, notification_buffer);
}
static void create_logo_layer(Window *window) {
	GRect logoBounds = logo_image->bounds;

	notification_layer = layer_create(GRect(0, bounds.size.h - logoBounds.size.h, bounds.size.w, logoBounds.size.h));
	layer_add_child(window_get_root_layer(window), notification_layer);

	// display logo
	logo_layer = bitmap_layer_create(GRect(3, 0, bounds.size.w, logoBounds.size.h));
	bitmap_layer_set_background_color(logo_layer, GColorWhite);
	bitmap_layer_set_alignment(logo_layer, GAlignTopLeft);
	bitmap_layer_set_bitmap(logo_layer, logo_image);
	layer_add_child(notification_layer, bitmap_layer_get_layer(logo_layer));

	// display notofications
	int offset = 4;
	notification_text_layer = text_layer_create(GRect(logoBounds.size.w + 7, offset, bounds.size.w - logoBounds.size.w - 7, logoBounds.size.h - offset));
	//text_layer_set_background_color(notification_text_layer, GColorClear);
	text_layer_set_font(notification_text_layer, statusFont);
	text_layer_set_text_alignment(notification_text_layer, GTextAlignmentLeft);
	text_layer_set_overflow_mode(notification_text_layer, GTextOverflowModeTrailingEllipsis);
	//text_layer_set_text(notification_text_layer, "noti");
	layer_add_child(notification_layer, text_layer_get_layer(notification_text_layer));

	// hide notification bar for state item, show for other items
	layer_set_hidden(notification_layer, (current_item == 0 ? true : false));
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
				text_layer_set_text(confirm_text_layer, navigation_text_start);
				break;
			/* connecting */
			case 1:
				text_layer_set_text(confirm_text_layer, navigation_text_cancel);
				break;
			/* connected */
			case 2:
				text_layer_set_text(confirm_text_layer, navigation_text_options);
				break;
		}
	}
	else
		text_layer_set_text(confirm_text_layer, navigation_text_log);

	// hide notification bar for state item, show for other items
	layer_set_hidden(notification_layer, (current_item == 0 ? true : false));
}
static void create_navi(Window *window) {
	navi_layer = layer_create(bounds);
	layer_set_update_proc(navi_layer, navi_layer_update_callback);
	layer_add_child(window_get_root_layer(window), navi_layer);

	// create action text layer
	confirm_text_layer = text_layer_create(GRect(0, (bounds.size.h - 34)/2, bounds.size.w - 4, 34));
	text_layer_set_font(confirm_text_layer, symbolFont);
	text_layer_set_background_color(confirm_text_layer, GColorClear);
	text_layer_set_text_alignment(confirm_text_layer, GTextAlignmentRight);
	text_layer_set_text(confirm_text_layer, navigation_text_log);
	layer_add_child(navi_layer, text_layer_get_layer(confirm_text_layer));
}

static void state_layer_update_callback(Layer *me, GContext* ctx) {
	if (state_text_layer_top == NULL || state_text_layer_bottom == NULL || lastState == state)
		return;
	
	char *topText = NULL, *bottomText = NULL;
	switch(state) {
		/* disconnected */
		case 0:
			topText = text_start_title;
			bottomText = text_start_subtitle;
			break;
		/* connecting */
		case 1:
			topText = text_start_options;
			bottomText = empty_text;
			break;
		/* connected */
		case 2:
			topText = text_start_options;
			bottomText = empty_text;
			break;
	}
	// resize title layer to center
	GSize size = graphics_text_layout_get_content_size(topText, titleFont, GRect(2, 0, bounds.size.w - 4, bounds.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter);
	layer_set_frame(text_layer_get_layer(state_text_layer_top), GRect(2, (bounds.size.h - size.h) / 2 - 2, bounds.size.w - 4, size.h));
	text_layer_set_text(state_text_layer_top, topText);
	text_layer_set_text(state_text_layer_bottom, bottomText);
	
	lastState = state;
}
static void create_state_layer(Window *window) {
	int posIndex = 0;
	if (current_item != 0)
		posIndex = 1;
	state_layer = layer_create(GRect(0, posIndex * bounds.size.h, bounds.size.w, bounds.size.h));
	layer_set_update_proc(state_layer, state_layer_update_callback);
	layer_add_child(window_get_root_layer(window), state_layer);

	state_text_layer_top = text_layer_create(GRect(2, 33, bounds.size.w - 4, bounds.size.h - 73));
	text_layer_set_background_color(state_text_layer_top, GColorClear);
	text_layer_set_font(state_text_layer_top, titleFont);
	text_layer_set_text_alignment(state_text_layer_top, GTextAlignmentCenter);
	//text_layer_set_text(state_text_layer_top, text_start_title);
	layer_add_child(state_layer, text_layer_get_layer(state_text_layer_top));

	state_text_layer_bottom = text_layer_create(GRect(2, bounds.size.h - 40, bounds.size.w - 4, 40));
	text_layer_set_background_color(state_text_layer_bottom, GColorClear);
	text_layer_set_font(state_text_layer_bottom, subtitleFont);
	text_layer_set_text_alignment(state_text_layer_bottom, GTextAlignmentCenter);
	//text_layer_set_text(state_text_layer_bottom, text_start_subtitle);
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
	
	if (events_layer == NULL) {
		events_layer = layer_create(GRect(0, -(current_item + 1) * bounds.size.h, bounds.size.w, bounds.size.h * (screen_count + 2)));
		layer_add_child(window_get_root_layer(window), events_layer);
	}
	else
		layer_remove_child_layers(events_layer);

	if (s_active_item_count > 0) {
		int offset = screen_count - s_active_item_count;
		int i;
		add_event_layer(eventsFont, s_active_item_count - 1, 0);
		for (i = 0; i < s_active_item_count; i++) {
			add_event_layer(eventsFont, i, i + 1 + offset);
		}
	}
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
	if (s_active_item_count == 0) {
		activity_append("cup of coffee", "keepzer.calendar.event", "{\"event\":\"cup of coffee\"}");
		activity_append("snack", "keepzer.calendar.event", "{\"event\":\"snack\"}");
		activity_append("workout", "keepzer.calendar.event", "{\"event\":\"workout\"}");
	}
		
	create_events(window);
	create_state_layer(window);
	create_logo_layer(window);
	create_navi(window);
	
	display_update_state();
}

static void deinit(Window *window) {
	store_config();
	destroy_property_animation(&prop_animation);
	destroy_property_animation(&state_layer_animation);

	layer_destroy(navi_layer);
	layer_destroy(events_layer);
	state_layer = NULL;
	
	unload_resources();
}

int main(void) {
	/* load fonts */
	symbolFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNICONS_30));
	eventsFont = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
	//titleFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONTSERRAT_BOLD_30));
	titleFont = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
	subtitleFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);
	statusFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);
	smallFont = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
	tinyFont = fonts_get_system_font(FONT_KEY_GOTHIC_18);
	
	/* create main window */
	window = window_create();
	window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
	window_set_fullscreen(window, true);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = init,
		.unload = deinit
	});
	window_stack_push(window, false);

	/* main event loop */
	app_event_loop();
	
	window_destroy(window);
	
	/* unload fonts */
	fonts_unload_custom_font(symbolFont);
	//fonts_unload_custom_font(titleFont);
}


void display_update_state() {
	if (state_layer == NULL) return;
	
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
		set_notification(notification_text_pending_one);
	else {
		char text[32];
		snprintf(text, 32, notification_text_pending, s_log_item_count);
		set_notification(text);
	}
}

void display_update_events() {
	create_events(window);
}


