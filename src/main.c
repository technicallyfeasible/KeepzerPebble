#include "pebble.h"
#include "mini-printf.h"
#include "messaging.h"
#include "storage.h"

#include <stdlib.h>
#include <string.h>


static Window *window;
static PropertyAnimation *prop_animation;

static GRect bounds;
static BitmapLayer *logo_layer = NULL;
static Layer *navi_layer = NULL;
static TextLayer *confirm_text_layer = NULL;
static TextLayer *status_text_layer = NULL;
static Layer *events_layer = NULL;

static GBitmap *arrow_up_image;
static GBitmap *arrow_down_image;
static GBitmap *logo_image;

static int screen_count = 1;



static void select_current_item() {
	send_item(current_item);
}

static void destroy_property_animation(PropertyAnimation **prop_animation) {
	if (*prop_animation == NULL) {
		return;
	}
	if (animation_is_scheduled((Animation*) *prop_animation)) {
		animation_unschedule((Animation*) *prop_animation);
	}
	property_animation_destroy(*prop_animation);
	*prop_animation = NULL;
}

static void show_event(int index) {
	int screen_count = s_active_item_count + 1;
	
	int height = bounds.size.h * (screen_count + 2);
	destroy_property_animation(&prop_animation);

	GRect from_rect = GRect(0, -(current_item + 1) * bounds.size.h, bounds.size.w, height);
	if(index < 0) {
		index = screen_count - 1;
		from_rect.origin.y = -(screen_count + 1) * bounds.size.h;
	}
	else if(index >= screen_count) {
		index = 0;
		from_rect.origin.y = 0;
	}
	GRect to_rect = GRect(0, -(index + 1) * bounds.size.h, bounds.size.w, height);
	prop_animation = property_animation_create_layer_frame(events_layer, &from_rect, &to_rect);
	animation_set_duration((Animation*) prop_animation, 400);
	animation_schedule((Animation*) prop_animation);

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

static void create_logo_layer(Window *window) {
	GRect logoBounds = logo_image->bounds;
	logo_layer = bitmap_layer_create((GRect) { .origin = { 3, bounds.size.h - logoBounds.size.h }, .size = logoBounds.size });
	bitmap_layer_set_bitmap(logo_layer, logo_image);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(logo_layer));

	GFont statusFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);

	int offset = 4;
	TextLayer *status_text_layer = text_layer_create(GRect(logoBounds.size.w + 7, bounds.size.h - logoBounds.size.h + offset, bounds.size.w - logoBounds.size.w - 7, logoBounds.size.h - offset));
	text_layer_set_background_color(status_text_layer, GColorClear);
	text_layer_set_font(status_text_layer, statusFont);
	text_layer_set_text_alignment(status_text_layer, GTextAlignmentLeft);
	text_layer_set_overflow_mode(status_text_layer, GTextOverflowModeTrailingEllipsis);
	text_layer_set_text(status_text_layer, "Not connected");
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(status_text_layer));
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
		if (strlen(s_key_token) == 0)
			text_layer_set_text(confirm_text_layer, "CONNECT");
		else
			text_layer_set_text(confirm_text_layer, "DISCONNECT");
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

static void add_state_layer(int posIndex) {
	GFont bigFont = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
	GFont smallFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);

	TextLayer *text_layer = text_layer_create(GRect(2, posIndex * bounds.size.h, bounds.size.w, bounds.size.h));
	text_layer_set_background_color(text_layer, GColorClear);
	text_layer_set_font(text_layer, bigFont);
	text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
	text_layer_set_text(text_layer, "Keepzer for Pebble");
	layer_add_child(events_layer, text_layer_get_layer(text_layer));

	text_layer = text_layer_create(GRect(2, posIndex * bounds.size.h + 24, bounds.size.w, (bounds.size.h / 2) - 24));
	text_layer_set_background_color(text_layer, GColorClear);
	text_layer_set_font(text_layer, smallFont);
	text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
	text_layer_set_text(text_layer, "Press CONNECT to connect to your Keepzer account");
	layer_add_child(events_layer, text_layer_get_layer(text_layer));
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
		add_state_layer(1);
		for (i = 0; i < s_active_item_count; i++) {
			add_event_layer(font, i, i + 1 + offset);
		}
		add_state_layer(screen_count + 1);
		//add_event_layer(font, 0, s_active_item_count + 1 + offset);
	}
	else
		add_state_layer(1);
}

static void config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) click_handler);
}

static void init(void) {
	window = window_create();
	window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
	window_set_fullscreen(window, true);
	window_stack_push(window, false);

	Layer *window_layer = window_get_root_layer(window);
	// Get the bounds of the window for sizing the text layer
	bounds = layer_get_bounds(window_layer);

	init_messaging();
	load_keytoken();
	load_config();

	load_resources();

	// check if we have a keytoken, if not then show the connection screen
	get_account_token();
	
	create_logo_layer(window);
	create_events(window);
	create_navi(window);
}

static void deinit(void) {
	store_config();
	destroy_property_animation(&prop_animation);

	window_stack_remove(window, false);
	window_destroy(window);
	layer_destroy(navi_layer);
	layer_destroy(events_layer);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}


void display_update_events() {
	create_events(window);
}
