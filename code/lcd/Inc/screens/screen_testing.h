#ifndef SCREEN_TESTING_H_
#define SCREEN_TESTING_H_

#include "input_events.h"
#include "screens.h"

void screen_testing_on_enter (void);
void screen_testing_on_exit (void);
void screen_testing_render (void);
ScreenID screen_testing_handle_event (InputEvent);

#endif /* SCREEN_TESTING_H_ */