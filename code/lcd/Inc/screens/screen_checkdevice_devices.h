#ifndef SCREEN_SETTINGS_DEVICES_H_
#define SCREEN_SETTINGS_DEVICES_H_

#include "input_events.h"
#include "screens.h"

void screen_settingsdevices_on_enter (void);
void screen_settingsdevices_on_exit (void);
void screen_settingsdevices_render (void);
ScreenID screen_settingsdevices_handle_event (InputEvent);

#endif /* SCREEN_SETTINGS_DEVICES_H_ */