#ifndef SCREEN_TESTRUNNING_H_
#define SCREEN_TESTRUNNING_H_

#include "input_events.h"
#include "screens.h"

void screen_testrunning_on_enter (void);
void screen_testrunning_on_exit (void);
void screen_testrunning_render (void);
ScreenID screen_testrunning_handle_event (InputEvent);

#endif /* SCREEN_TESTRUNNING_H_ */