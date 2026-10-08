#ifndef SCREEN_CHECKFAILED_H_
#define SCREEN_CHECKFAILED_H_

#include "input_events.h"
#include "screens.h"

void screen_checkfailed_on_enter (void);
void screen_checkfailed_on_exit (void);
void screen_checkfailed_render (void);
ScreenID screen_checkfailed_handle_event (InputEvent);

#endif /* SCREEN_CHECKFAILED_H_ */