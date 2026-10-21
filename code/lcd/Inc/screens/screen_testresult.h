#ifndef SCREEN_TESTRESULT_H_
#define SCREEN_TESTRESULT_H_

#include "input_events.h"
#include "screens.h"

void screen_testresult_on_enter (void);
void screen_testresult_on_exit (void);
void screen_testresult_render (void);
ScreenID screen_testresult_handle_event (InputEvent);

#endif /* SCREEN_TESTRESULT_H_ */