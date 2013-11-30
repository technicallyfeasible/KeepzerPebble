#include "pebble.h"
#include "pebble_fonts.h"

#include <stdlib.h>

#define MAX_ACTIVITY_ITEMS (20)
#define MAX_ITEM_TEXT_LENGTH (32)

static Window *window;
static TextLayer *text_layer;
static PropertyAnimation *prop_animation;

static GRect bounds;
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


static void activity_append(char *data) {
  if (s_active_item_count == MAX_ACTIVITY_ITEMS) { 
    return;
  }
  strcpy(s_activity_items[s_active_item_count].name, data);
  s_active_item_count++;
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
  destroy_property_animation(&prop_animation);
  if(index < 0) {
    index = s_active_item_count - 1;
    layer_set_bounds(events_layer, GRect(0, -s_active_item_count * bounds.size.h, bounds.size.w, bounds.size.h * s_active_item_count));
  }
  if(index >= s_active_item_count) {
    index = 0;
    layer_set_bounds(events_layer, GRect(0, 1 * bounds.size.h, bounds.size.w, bounds.size.h * s_active_item_count));
  }
  //layer_set_bounds(events_layer, GRect(0, -current_item * bounds.size.h, bounds.size.w, bounds.size.h * s_active_item_count));

  GRect to_rect = GRect(0, -index * bounds.size.h, bounds.size.w, bounds.size.h * s_active_item_count);
  prop_animation = property_animation_create_layer_frame(events_layer, NULL, &to_rect);
  animation_set_duration((Animation*) prop_animation, 400);
  animation_schedule((Animation*) prop_animation);

  current_item = index;
}

static void click_handler(ClickRecognizerRef recognizer, Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Navigate");

  int next_item = current_item;
  switch (click_recognizer_get_button_id(recognizer)) {
    case BUTTON_ID_UP:
	  next_item = current_item - 1;
      break;

    case BUTTON_ID_DOWN:
	  next_item = current_item + 1;
      break;

    default:
    case BUTTON_ID_SELECT:
      break;
  }
  show_event(next_item);
}

static void load_resources() {
  arrow_up_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW_UP);
  arrow_down_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW_DOWN);
  logo_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);
}

static void navi_layer_update_callback(Layer *me, GContext* ctx) {
  GRect arrowBounds = arrow_up_image->bounds;
  graphics_draw_bitmap_in_rect(ctx, arrow_up_image, (GRect) { .origin = { bounds.size.w - arrowBounds.size.w, 0 }, .size = arrowBounds.size });
  arrowBounds = arrow_down_image->bounds;
  graphics_draw_bitmap_in_rect(ctx, arrow_down_image, (GRect) { .origin = { bounds.size.w - arrowBounds.size.w, bounds.size.h - arrowBounds.size.h }, .size = arrowBounds.size });
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
  text_layer_set_text(confirm_text_layer, "log");
}

static void events_layer_update_callback(Layer *me, GContext* ctx) {
  GRect logoBounds = logo_image->bounds;
  graphics_draw_bitmap_in_rect(ctx, logo_image, (GRect) { .origin = { 0, bounds.size.h - logoBounds.size.h }, .size = logoBounds.size });
}
static void add_event_layer(GFont font, int itemIndex, int posIndex) {
  TextLayer *text_layer = text_layer_create(GRect(2, posIndex * bounds.size.h, bounds.size.w - 22, bounds.size.h));
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_font(text_layer, font);
  text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
  text_layer_set_text(text_layer, s_activity_items[itemIndex].name);
  layer_add_child(events_layer, text_layer_get_layer(text_layer));
}
static void create_events(Window *window) {
  events_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h * s_active_item_count));
  layer_set_update_proc(events_layer, events_layer_update_callback);
  layer_add_child(window_get_root_layer(window), events_layer);

  GFont font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);

/*  int i;
  add_event_layer(events_layer, s_active_item_count - 1, -1);
  for (i = 0; i < s_active_item_count; i++) {
    add_event_layer(events_layer, i, i);
  }
  add_event_layer(font, i, s_active_item_count);*/
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

  activity_append("Drank 1 glass of water");
  activity_append("Brushed teeth");
  activity_append("Ate a muffin");

  load_resources();
  create_events(window);
  create_navi(window);

  //prop_animation = property_animation_create_layer_frame(text_layer_get_layer(text_layer), NULL, &to_rect);

  //animation_schedule((Animation*) prop_animation);
}

static void deinit(void) {
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
