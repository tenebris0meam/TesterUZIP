#ifndef SCREEN_SETTINGS_SERIES_H_
#define SCREEN_SETTINGS_SERIES_H_

#include "input_events.h"
#include "screens.h"

void screen_settingsseries_on_enter (void);
void screen_settingsseries_on_exit (void);
void screen_settingsseries_render (void);
ScreenID screen_settingsseries_handle_event (InputEvent);

#endif /* SCREEN_SETTINGS_SERIES_H_ */