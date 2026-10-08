#ifndef SCREEN_SETTINGS_INTERFACE_H_
#define SCREEN_SETTINGS_INTERFACE_H_

#include "input_events.h"
#include "screens.h"

void screen_settingsinterface_on_enter (void);
void screen_settingsinterface_on_exit (void);
void screen_settingsinterface_render (void);
ScreenID screen_settingsinterface_handle_event (InputEvent);

#endif /* SCREEN_SETTINGS_INTERFACE_H_ */