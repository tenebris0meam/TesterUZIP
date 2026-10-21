#ifndef SCREEN_CHECKDEVICE_GROUPS_H_
#define SCREEN_CHECKDEVICE_GROUPS_H_

#include "input_events.h"
#include "screens.h"

void screen_checkdevice_groups_on_enter (void);
void screen_checkdevice_groups_on_exit (void);
void screen_checkdevice_groups_render (void);
ScreenID screen_checkdevice_groups_handle_event (InputEvent);

#endif /* SCREEN_CHECKDEVICE_GROUPS_H_ */