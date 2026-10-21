#ifndef SCREEN_CHECKDEVICE_DEVICES_H_
#define SCREEN_CHECKDEVICE_DEVICES_H_

#include "input_events.h"
#include "screens.h"

void screen_checkdevice_devices_on_enter (void);
void screen_checkdevice_devices_on_exit (void);
void screen_checkdevice_devices_render (void);
ScreenID screen_checkdevice_devices_handle_event (InputEvent);

#endif /* SCREEN_CHECKDEVICE_DEVICES_H_ */