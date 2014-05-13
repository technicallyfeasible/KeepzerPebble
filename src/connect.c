#include "pebble.h"
#include "display.h"
#include "connect.h"
#include "storage.h"
#include "messaging.h"

static Window *connect_window = NULL;
static TextLayer *navi_text_layer = NULL;
static Layer *step1_layer = NULL;
static TextLayer *step1_layer_text = NULL;
static Layer *step2_layer = NULL;
static TextLayer *step2_layer_text = NULL;
static TextLayer *step2_layer_text_bottom = NULL;
static Layer *step3_layer = NULL;
static TextLayer *step3_layer_text = NULL;
static Layer *step4_layer = NULL;
static TextLayer *step4_layer_text = NULL;
static Layer *step5_layer = NULL;
static TextLayer *step5_layer_text = NULL;
static TextLayer *code_text_layer = NULL;
static Layer *step6_layer = NULL;
static TextLayer *step6_layer_text = NULL;
static TextLayer *step6_layer_text_bottom = NULL;

static int step = 0;

static char *text_next = "3";
static char *text_finish = "c";
static char *text_connect_step1 = "Connect your phone\nto the Internet";
static char *text_connect_step2 = "Sign in or sign up at:";
static char *text_connect_step2_bottom = "keepzer.com";
static char *text_connect_step3 = "Click:\nConnect more trackers";
static char *text_connect_step4 = "Click:\nKeepzer for Pebble";
static char *text_connect_step5 = "Enter:";
static char *text_connect_step5_code = "Loading code...";
static char *text_connect_step6 = "Success";
static char *text_connect_step6_bottom = "Your watch is now connected. Create your own events in the config page on your phone.";

static PropertyAnimation *out_animation;
static PropertyAnimation *in_animation;


static void create_navi(Window *window) {
	// create action text layer
	navi_text_layer = text_layer_create(GRect(0, (bounds.size.h - 34)/2, bounds.size.w - 4, 34));
	text_layer_set_font(navi_text_layer, symbolFont);
	text_layer_set_background_color(navi_text_layer, GColorClear);
	text_layer_set_text_alignment(navi_text_layer, GTextAlignmentRight);
	text_layer_set_text(navi_text_layer, text_next);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(navi_text_layer));
}

static void create_step1(Window *window) {
	step1_layer = layer_create(bounds);
	layer_add_child(window_get_root_layer(window), step1_layer);
	
	step1_layer_text = text_layer_create(GRect(0, 0, bounds.size.w - 8, bounds.size.h));
	text_layer_set_background_color(step1_layer_text, GColorClear);
	text_layer_set_font(step1_layer_text, titleFont);
	text_layer_set_text_alignment(step1_layer_text, GTextAlignmentCenter);
	text_layer_set_text(step1_layer_text, text_connect_step1);
	layer_add_child(step1_layer, text_layer_get_layer(step1_layer_text));
}

static void create_step2(Window *window) {
	step2_layer = layer_create(GRect(bounds.size.w, 0, bounds.size.w, bounds.size.h));
	layer_add_child(window_get_root_layer(window), step2_layer);
	
	step2_layer_text = text_layer_create(GRect(0, 10, bounds.size.w - 8, bounds.size.h - 50));
	text_layer_set_background_color(step2_layer_text, GColorClear);
	text_layer_set_font(step2_layer_text, titleFont);
	text_layer_set_text_alignment(step2_layer_text, GTextAlignmentCenter);
	text_layer_set_text(step2_layer_text, text_connect_step2);
	layer_add_child(step2_layer, text_layer_get_layer(step2_layer_text));

	step2_layer_text_bottom = text_layer_create(GRect(0, bounds.size.h - 45, bounds.size.w - 8, 45));
	text_layer_set_background_color(step2_layer_text_bottom, GColorClear);
	text_layer_set_font(step2_layer_text_bottom, smallFont);
	text_layer_set_text_alignment(step2_layer_text_bottom, GTextAlignmentCenter);
	text_layer_set_text(step2_layer_text_bottom, text_connect_step2_bottom);
	layer_add_child(step2_layer, text_layer_get_layer(step2_layer_text_bottom));
}

static void create_step3(Window *window) {
	step3_layer = layer_create(GRect(bounds.size.w, 0, bounds.size.w, bounds.size.h));
	layer_add_child(window_get_root_layer(window), step3_layer);
	
	step3_layer_text = text_layer_create(GRect(0, 3, bounds.size.w - 8, bounds.size.h));
	text_layer_set_background_color(step3_layer_text, GColorClear);
	text_layer_set_font(step3_layer_text, titleFont);
	text_layer_set_text_alignment(step3_layer_text, GTextAlignmentCenter);
	text_layer_set_text(step3_layer_text, text_connect_step3);
	layer_add_child(step3_layer, text_layer_get_layer(step3_layer_text));
}

static void create_step4(Window *window) {
	step4_layer = layer_create(GRect(bounds.size.w, 0, bounds.size.w, bounds.size.h));
	layer_add_child(window_get_root_layer(window), step4_layer);
	
	step4_layer_text = text_layer_create(GRect(0, 3, bounds.size.w - 8, bounds.size.h));
	text_layer_set_background_color(step4_layer_text, GColorClear);
	text_layer_set_font(step4_layer_text, titleFont);
	text_layer_set_text_alignment(step4_layer_text, GTextAlignmentCenter);
	text_layer_set_text(step4_layer_text, text_connect_step4);
	layer_add_child(step4_layer, text_layer_get_layer(step4_layer_text));
}

static void create_step5(Window *window) {
	step5_layer = layer_create(GRect(bounds.size.w, 0, bounds.size.w, bounds.size.h));
	layer_add_child(window_get_root_layer(window), step5_layer);
	
	step5_layer_text = text_layer_create(GRect(0, 30, bounds.size.w, bounds.size.h - 105));
	text_layer_set_background_color(step5_layer_text, GColorClear);
	text_layer_set_font(step5_layer_text, titleFont);
	text_layer_set_text_alignment(step5_layer_text, GTextAlignmentCenter);
	text_layer_set_text(step5_layer_text, text_connect_step5);
	layer_add_child(step5_layer, text_layer_get_layer(step5_layer_text));

	code_text_layer = text_layer_create(GRect(0, bounds.size.h - 65, bounds.size.w - 8, 65));
	text_layer_set_background_color(code_text_layer, GColorClear);
	text_layer_set_font(code_text_layer, codeFont);
	text_layer_set_text_alignment(code_text_layer, GTextAlignmentCenter);
	text_layer_set_text(code_text_layer, text_connect_step5_code);
	layer_add_child(step5_layer, text_layer_get_layer(code_text_layer));
}

static void create_step6(Window *window) {
	step6_layer = layer_create(GRect(bounds.size.w, 0, bounds.size.w, bounds.size.h));
	layer_add_child(window_get_root_layer(window), step6_layer);
	
	step6_layer_text = text_layer_create(GRect(0, 10, bounds.size.w - 8, 30));
	text_layer_set_background_color(step6_layer_text, GColorClear);
	text_layer_set_font(step6_layer_text, titleFont);
	text_layer_set_text_alignment(step6_layer_text, GTextAlignmentCenter);
	text_layer_set_text(step6_layer_text, text_connect_step6);
	layer_add_child(step6_layer, text_layer_get_layer(step6_layer_text));

	step6_layer_text_bottom = text_layer_create(GRect(8, 45, bounds.size.w - 20, bounds.size.h - 45));
	text_layer_set_background_color(step6_layer_text_bottom, GColorClear);
	text_layer_set_font(step6_layer_text_bottom, tinyFont);
	text_layer_set_text_alignment(step6_layer_text_bottom, GTextAlignmentLeft);
	text_layer_set_text(step6_layer_text_bottom, text_connect_step6_bottom);
	layer_add_child(step6_layer, text_layer_get_layer(step6_layer_text_bottom));
}

static Layer *get_layer(int step) {
	switch(step) {
		case 0:
			return step1_layer;
		case 1:
			return step2_layer;
		case 2:
			return step3_layer;
		case 3:
			return step4_layer;
		case 4:
			return step5_layer;
		case 5:
			return step6_layer;
	}
	return NULL;
}

static void set_step(int nextStep) {
	if (nextStep == step || nextStep < 0 || nextStep > 5) return;

	destroy_property_animation(&out_animation);
	destroy_property_animation(&in_animation);

	Layer *current_layer = get_layer(step);
	Layer *next_layer = get_layer(nextStep);

	if (current_layer != NULL) {
		GRect from_rect = GRect(0, 0, bounds.size.w, bounds.size.h);
		GRect to_rect = GRect(-bounds.size.w, 0, bounds.size.w, bounds.size.h);
		out_animation = property_animation_create_layer_frame(current_layer, &from_rect, &to_rect);
		animation_set_duration((Animation*) out_animation, 400);
		//animation_set_handlers((Animation*) out_animation, (AnimationHandlers) { .stopped = (AnimationStoppedHandler) animation_done }, NULL);
		animation_schedule((Animation*) out_animation);
	}
	if (next_layer != NULL) {
		GRect from_rect = GRect(bounds.size.w, 0, bounds.size.w, bounds.size.h);
		GRect to_rect = GRect(0, 0, bounds.size.w, bounds.size.h);
		in_animation = property_animation_create_layer_frame(next_layer, &from_rect, &to_rect);
		animation_set_duration((Animation*) in_animation, 400);
		//animation_set_handlers((Animation*) in_animation, (AnimationHandlers) { .stopped = (AnimationStoppedHandler) animation_done }, NULL);
		animation_schedule((Animation*) in_animation);
	}
	text_layer_set_text(navi_text_layer, (nextStep >= 5 ? text_finish : nextStep == 4 ? "" : text_next));
	step = nextStep;
}

static void navigate_next() {
	/* close the process when we have a token and are after the success display */
	if (strlen(s_key_token) > 0 && step == 5) {
		window_stack_pop(true);
		return;
	}
	
	/* while waiting for connection cannot move to another step */
	if (step > 4 || (step == 4 && strlen(s_key_token) == 0)) return;
	if (step == 3) {
		connect();
		connect_update_state();
	}
	set_step(step + 1);
}

static void connect_click_handler(ClickRecognizerRef recognizer, Window *window) {
	switch (click_recognizer_get_button_id(recognizer)) {
		case BUTTON_ID_UP:
			break;
		case BUTTON_ID_DOWN:
			break;
		default:
		case BUTTON_ID_SELECT:
			navigate_next();
			break;
	}
}

/* configure click handlers */
static void connect_config_provider(Window *window) {
	//window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) connect_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) connect_click_handler);
	//window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) connect_click_handler);
}

/* initialize the settings */
static void init_connect(Window *window) {
	step = 0;
	create_step1(window);
	create_step2(window);
	create_step3(window);
	create_step4(window);
	create_step5(window);
	create_step6(window);
	create_navi(window);
}

/* deinitialize the settings */
static void deinit_connect(Window *window) {
	// if we have a keytoken we don't have to cancel anymore
	if (strlen(s_key_token) == 0)
		cancel_connect();
	step = 0;
	
	destroy_property_animation(&out_animation);
	destroy_property_animation(&in_animation);

	text_layer_destroy(navi_text_layer);
	layer_destroy(step1_layer);
	text_layer_destroy(step1_layer_text);
	layer_destroy(step2_layer);
	text_layer_destroy(step2_layer_text);
	text_layer_destroy(step2_layer_text_bottom);
	layer_destroy(step3_layer);
	text_layer_destroy(step3_layer_text);
	layer_destroy(step4_layer);
	text_layer_destroy(step4_layer_text);
	layer_destroy(step5_layer);
	text_layer_destroy(step5_layer_text);
	text_layer_destroy(code_text_layer);
	layer_destroy(step6_layer);
	text_layer_destroy(step6_layer_text);
	text_layer_destroy(step6_layer_text_bottom);

	code_text_layer = NULL;
}


/* start the connection process */
void connect_start() {
	step = 0;
	if (connect_window == NULL) {
		connect_window = window_create();
		window_set_click_config_provider(connect_window, (ClickConfigProvider) connect_config_provider);
		window_set_fullscreen(connect_window, true);
		window_set_window_handlers(connect_window, (WindowHandlers) {
			.load = init_connect,
			.unload = deinit_connect
		});
	}
	window_stack_push(connect_window, true);
}
/* destroy connect process resources */
void connect_destroy() {
	if (connect_window == NULL)
		return;
	window_destroy(connect_window);
}

/* update the connection state */
void connect_update_state() {
	if (code_text_layer == NULL) return;

	text_layer_set_text(code_text_layer, s_sensor_id);
	
	/* we got a keytoken so show the success page */
	if (strlen(s_key_token) > 0 && step == 4) {
		navigate_next();
	}
}
