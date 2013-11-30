#include "pebble.h"

#include <stdlib.h>

static Window *window;
static TextLayer *text_layer;
static PropertyAnimation *prop_animation;

static GRect bounds;
static Layer *navi_layer;
static TextLayer *confirm_text_layer;
static TextLayer *sync_text;
static TextLayer *item_text_1;
static TextLayer *item_text_2;

static GBitmap *arrow_up_image;
static GBitmap *arrow_down_image;

static void animation_started(Animation *animation, void *data) {
  //text_layer_set_text(text_layer, "Started.");
}

static void animation_stopped(Animation *animation, bool finished, void *data) {
  //text_layer_set_text(text_layer, finished ? "Hi, I'm a TextLayer!" : "Just Stopped.");
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

static void click_handler(ClickRecognizerRef recognizer, Window *window) {
  /*Layer *layer = text_layer_get_layer(text_layer);

  GRect to_rect;
  if (toggle) {
    to_rect = GRect(4, 4, 120, 60);
  } else {
    to_rect = GRect(84, 92, 60, 60);
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Toggle is %u, %p", toggle, &toggle);

  destroy_property_animation(&prop_animation);

  prop_animation = property_animation_create_layer_frame(layer, NULL, &to_rect);
  animation_set_duration((Animation*) prop_animation, 400);
  switch (click_recognizer_get_button_id(recognizer)) {
    case BUTTON_ID_UP:
      animation_set_curve((Animation*) prop_animation, AnimationCurveEaseOut);
      break;

    case BUTTON_ID_DOWN:
      animation_set_curve((Animation*) prop_animation, AnimationCurveEaseIn);
      break;

    default:
    case BUTTON_ID_SELECT:
      animation_set_curve((Animation*) prop_animation, AnimationCurveEaseInOut);
      break;
  }

  animation_set_handlers((Animation*) prop_animation, (AnimationHandlers) {
    .started = (AnimationStartedHandler) animation_started,
    .stopped = (AnimationStoppedHandler) animation_stopped,
  }, NULL);
  animation_schedule((Animation*) prop_animation);*/
}

static void load_resources() {
  arrow_up_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW_UP);
  arrow_down_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW_DOWN);
}

static void create_navi(Window *window) {
  navi_layer = layer_create(GRect(0,0, 48, bounds.size.h));
  
  confirm_text_layer = text_layer_create(GRect(0, 0, 48, bounds.size.h));
  text_layer_set_text(confirm_text_layer, "log");
  layer_add_child(navi_layer, text_layer_get_layer(confirm_text_layer));


  layer_add_child(window_get_root_layer(window), navi_layer);
}

static void config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) click_handler);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
  window_stack_push(window, false);

  Layer *window_layer = window_get_root_layer(window);
  // Get the bounds of the window for sizing the text layer
  bounds = layer_get_bounds(window_layer);

  load_resources();
  create_navi(window);

  GRect to_rect = GRect(84, 92, 60, 60);

  //prop_animation = property_animation_create_layer_frame(text_layer_get_layer(text_layer), NULL, &to_rect);

  //animation_schedule((Animation*) prop_animation);
}

static void deinit(void) {
  destroy_property_animation(&prop_animation);

  window_stack_remove(window, false);
  window_destroy(window);
  text_layer_destroy(text_layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
