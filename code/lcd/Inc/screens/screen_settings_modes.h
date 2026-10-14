#ifndef SCREEN_SETTINGS_MODES_H_
#define SCREEN_SETTINGS_MODES_H_

#include "screens.h"
#include "input_events.h"

void screen_settingsmodes_on_enter (void);
void screen_settingsmodes_render (void);
void screen_settingsmodes_on_exit (void);
ScreenID screen_settingsmodes_handle_event (InputEvent event);

#endif /* SCREEN_SETTINGS_MODES_H_ */