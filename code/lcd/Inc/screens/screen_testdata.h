#ifndef SCREEN_TESTDATA_H_
#define SCREEN_TESTDATA_H_

#include "input_events.h"
#include "screens.h"

void screen_testdata_on_enter (void);
void screen_testdata_on_exit (void);
void screen_testdata_render (void);
ScreenID screen_testdata_handle_event (InputEvent);

#endif /* SCREEN_TESTDATA_H_ */