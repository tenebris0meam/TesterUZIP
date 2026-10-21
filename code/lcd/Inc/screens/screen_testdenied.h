#ifndef SCREEN_TESTDENIED_H_
#define SCREEN_TESTDENIED_H_

#include "input_events.h"
#include "screens.h"

void screen_testdenied_on_enter (void);
void screen_testdenied_on_exit (void);
void screen_testdenied_render (void);
ScreenID screen_testdenied_handle_event (InputEvent);

#endif /* SCREEN_TESTDENIED_H_ */