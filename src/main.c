#include "pebble.h"
#include "mini-printf.h"

#include <stdlib.h>
#include <string.h>

#define MAX_ACTIVITY_ITEMS (20)
#define MAX_ITEM_TEXT_LENGTH (32)

#define MESSAGE_TYPE 0
#define MESSAGE_ITEM 10
#define MESSAGE_ITEM_NAME 11
#define MESSAGE_ITEM_COUNT 12
#define MESSAGE_MESSAGE 100
#define STORAGE_ITEM_COUNT 0
#define STORAGE_ITEMS 1
#define STORAGE_ITEM_CURRENT 2
#define STORAGE_KEYTOKEN 3

static const char *message_type_log = "log";
static const char *message_type_item = "item";
static const char *message_type_state = "state";
static const char *message_type_message = "message";

static Window *window;
static PropertyAnimation *prop_animation;

static GRect bounds;
static BitmapLayer *logo_layer;
static Layer *navi_layer;
static TextLayer *confirm_text_layer;
static Layer *events_layer;

static GBitmap *arrow_up_image;
static GBitmap *arrow_down_image;
static GBitmap *logo_image;

typedef struct {
  char name[MAX_ITEM_TEXT_LENGTH];
} ActivityItem;
static ActivityItem s_activity_items[MAX_ACTIVITY_ITEMS];
static int s_active_item_count = 0;
static int current_item = 0;
static char keyToken[128] = "\0";

static int s_screen_count = 1;


static void send_log(char *text) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_message);
	dict_write_cstring(iter, MESSAGE_MESSAGE, text);
	app_message_outbox_send();
}
static void script_log(char *format, int arg) {
	char text[128];
	mini_snprintf(text, 128, format, arg);
	send_log(text);
}
static void script_log2(char *format, int arg1, int arg2) {
	char text[160];
	mini_snprintf(text, 160, format, arg1, arg2);
	send_log(text);
}
static void script_log3(char *format, int arg1, int arg2, int arg3) {
	char text[192];
	mini_snprintf(text, 192, format, arg1, arg2, arg3);
	send_log(text);
}
static void script_log4(char *format, int arg1, int arg2, int arg3, int arg4) {
	char text[256];
	mini_snprintf(text, 256, format, arg1, arg2, arg3, arg4);
	send_log(text);
}

static void activity_append(char *data) {
	if (s_active_item_count >= MAX_ACTIVITY_ITEMS) { 
		return;
	}
	strcpy(s_activity_items[s_active_item_count].name, data);
	s_active_item_count++;
}
static void activity_set(int index, char *data) {
	if (index >= s_active_item_count || index >= MAX_ACTIVITY_ITEMS) { 
		return;
	}
	strcpy(s_activity_items[index].name, data);
}
/* Store all activities in persistent storage */
static void store_config() {
	persist_write_int(STORAGE_ITEM_CURRENT, current_item);
	persist_write_int(STORAGE_ITEM_COUNT, s_active_item_count);
	persist_write_data(STORAGE_ITEMS, s_activity_items, sizeof(ActivityItem)*s_active_item_count);
}
/* Load all activities in persistent storage */
static void load_config() {
	s_active_item_count = persist_read_int(STORAGE_ITEM_COUNT);
	current_item = persist_read_int(STORAGE_ITEM_CURRENT);
	if (current_item >= s_active_item_count)
		current_item = s_active_item_count;
	s_screen_count = s_active_item_count + 1;
	if (s_active_item_count == 0)
		return;
    if (s_active_item_count > MAX_ACTIVITY_ITEMS)
	  s_active_item_count = MAX_ACTIVITY_ITEMS;
	s_screen_count = s_active_item_count + 1;
	persist_read_data(STORAGE_ITEMS, s_activity_items, sizeof(ActivityItem)*s_active_item_count);
	
	// get items from the configuration
	if(s_active_item_count == 0)
	{
		
	}
}
/* Store the keytoken */
static void store_keytoken() {
	persist_write_string(STORAGE_KEYTOKEN, keyToken);
}
/* Load the keytoken */
static void load_keytoken() {
	persist_read_string(STORAGE_KEYTOKEN, keyToken, sizeof(keyToken));
}

static void select_current_item() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_log);
	dict_write_int16(iter, MESSAGE_ITEM, current_item);
	app_message_outbox_send();
}

static void get_connection_state() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_TYPE, message_type_state);
	app_message_outbox_send();
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
	int height = bounds.size.h * (s_screen_count + 2);
	destroy_property_animation(&prop_animation);

	GRect from_rect = GRect(0, -(current_item + 1) * bounds.size.h, bounds.size.w, height);
	if(index < 0) {
		index = s_screen_count - 1;
		from_rect.origin.y = -(s_screen_count + 1) * bounds.size.h;
	}
	else if(index >= s_screen_count) {
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
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Navigate");

	int next_item = current_item;
	switch (click_recognizer_get_button_id(recognizer)) {
		case BUTTON_ID_UP:
			next_item = current_item - 1;
			show_event(next_item);
		break;

		case BUTTON_ID_DOWN:
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
}

static void navi_layer_update_callback(Layer *me, GContext* ctx) {
	GRect arrowBounds = arrow_up_image->bounds;
	graphics_draw_bitmap_in_rect(ctx, arrow_up_image, (GRect) { .origin = { bounds.size.w - arrowBounds.size.w, 0 }, .size = arrowBounds.size });
	arrowBounds = arrow_down_image->bounds;
	graphics_draw_bitmap_in_rect(ctx, arrow_down_image, (GRect) { .origin = { bounds.size.w - arrowBounds.size.w, bounds.size.h - arrowBounds.size.h }, .size = arrowBounds.size });
	if(s_screen_count > 1) {
		int length = bounds.size.h / s_screen_count;
		int y = bounds.size.h * current_item / s_screen_count;
		graphics_draw_rect(ctx, GRect(bounds.size.w - 2, y, 2, length));
	}
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

static void add_state_layer(GFont font, int posIndex) {
	TextLayer *text_layer = text_layer_create(GRect(2, posIndex * bounds.size.h, bounds.size.w - 26, bounds.size.h));
	text_layer_set_background_color(text_layer, GColorClear);
	text_layer_set_font(text_layer, font);
	text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
	text_layer_set_text(text_layer, "Config");
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
	events_layer = layer_create(GRect(0, -(current_item + 1) * bounds.size.h, bounds.size.w, bounds.size.h * (s_screen_count + 2)));
	layer_add_child(window_get_root_layer(window), events_layer);

	GFont font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
	
	if(s_active_item_count > 0) {
		int offset = s_screen_count - s_active_item_count;
		int i;
		add_event_layer(font, s_active_item_count - 1, 0);
		add_state_layer(font, 1);
		for (i = 0; i < s_active_item_count; i++) {
			add_event_layer(font, i, i + 1 + offset);
		}
		add_state_layer(font, s_screen_count + 1);
		//add_event_layer(font, 0, s_active_item_count + 1 + offset);
	}
	else
		add_state_layer(font, 1);
}

static void config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) click_handler);
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
}
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
}
void in_received_handler(DictionaryIterator *iter, void *context) {
    // incoming message received
	// Check for fields you expect to receive
	Tuple *type_tuple = dict_find(iter, MESSAGE_TYPE);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Received app message");
	APP_LOG(APP_LOG_LEVEL_DEBUG, type_tuple->value->cstring);

	// Received items
    if (strcmp(type_tuple->value->cstring, message_type_item) == 0) {
		Tuple *item_tuple = dict_find(iter, MESSAGE_ITEM);
		Tuple *name_tuple = dict_find(iter, MESSAGE_ITEM_NAME);
		Tuple *count_tuple = dict_find(iter, MESSAGE_ITEM_COUNT);
		s_active_item_count = count_tuple->value->uint16;
		s_screen_count = s_active_item_count + 1;
		activity_set(item_tuple->value->uint16, name_tuple->value->cstring);
		layer_destroy(events_layer);
		create_events(window);
		store_config();
    }
}
void in_dropped_handler(AppMessageResult reason, void *context) {
	// incoming message dropped
}
static void init_messaging() {
	// Reduce the sniff interval for more responsive messaging at the expense of
	// increased energy consumption by the Bluetooth module
	// The sniff interval will be restored by the system after the app has been
	// unloaded
	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);

	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);

	const uint32_t inbound_size = 128;
	const uint32_t outbound_size = 128;
	app_message_open(inbound_size, outbound_size);
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
