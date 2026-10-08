#ifndef SCREEN_SETTINGS_H_
#define SCREEN_SETTINGS_H_

#include "input_events.h"
#include "screens.h"

void screen_settings_on_enter (void);
void screen_settings_on_exit (void);
void screen_settings_render (void);
ScreenID screen_settings_handle_event (InputEvent);

#endif /* SCREEN_SETTINGS_H_ */