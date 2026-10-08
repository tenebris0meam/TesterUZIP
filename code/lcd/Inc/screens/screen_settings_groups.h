#ifndef SCREEN_SETTINGS_GROUPS_H_
#define SCREEN_SETTINGS_GROUPS_H_

#include "input_events.h"
#include "screens.h"

void screen_settingsgroups_on_enter (void);
void screen_settingsgroups_on_exit (void);
void screen_settingsgroups_render (void);
ScreenID screen_settingsgroups_handle_event (InputEvent);

#endif /* SCREEN_SETTINGS_GROUPS_H_ */