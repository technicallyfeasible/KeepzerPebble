/*
 * Display stuff
 */


#ifndef __KEEPZER_DISPLAY__
#define __KEEPZER_DISPLAY__

//#define LOG(text) APP_LOG(APP_LOG_LEVEL_DEBUG, text)
#define LOG(text) 
	
extern GRect bounds;

extern GBitmap *arrow_up_image, *arrow_down_image, *logo_image, *icon_info, *icon_disconnect;
extern GFont symbolFont, titleFont, subtitleFont, eventsFont, statusFont, smallFont, tinyFont;

void destroy_property_animation(PropertyAnimation **animation);
void animation_done(struct Animation *animation, bool finished, void *context);
void display_update_state();
void display_update_events();

#endif
