#ifndef SCREEN_CHECKDEVICE_SERIES_H_
#define SCREEN_CHECKDEVICE_SERIES_H_

#include "input_events.h"
#include "screens.h"

void screen_checkdevice_series_on_enter (void);
void screen_checkdevice_series_on_exit (void);
void screen_checkdevice_series_render (void);
ScreenID screen_checkdevice_series_handle_event (InputEvent);

#endif /* SCREEN_CHECKDEVICE_SERIES_H_ */