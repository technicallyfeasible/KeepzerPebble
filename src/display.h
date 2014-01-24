/*
 * Display stuff
 */


#ifndef __KEEPZER_DISPLAY__
#define __KEEPZER_DISPLAY__

extern GRect bounds;

extern GFont symbolFont, titleFont, subtitleFont, eventsFont, statusFont;

void display_update_state();
void display_update_events();

#endif
