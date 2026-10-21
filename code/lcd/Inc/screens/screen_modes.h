#ifndef SCREEN_MODES_H_
#define SCREEN_MODES_H_

#include "screens.h"
#include "input_events.h"

void screen_modes_on_enter (void);
void screen_modes_render (void);
void screen_modes_on_exit (void);
ScreenID screen_modes_handle_event (InputEvent event);

#endif /* SCREEN_MODES_H_ */