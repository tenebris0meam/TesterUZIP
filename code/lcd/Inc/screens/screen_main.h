#ifndef SCREEN_MAIN_H_
#define SCREEN_MAIN_H_

#include "input_events.h"
#include "screens.h"

void screen_main_on_enter (void);
void screen_main_on_exit (void);
void screen_main_render (void);
ScreenID screen_main_handle_event (InputEvent);

#endif /* SCREEN_MAIN_H_ */