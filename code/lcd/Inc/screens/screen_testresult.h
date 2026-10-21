#ifndef SCREEN_RESULT_H_
#define SCREEN_RESULT_H_

#include "input_events.h"
#include "screens.h"

void screen_result_on_enter (void);
void screen_result_on_exit (void);
void screen_result_render (void);
ScreenID screen_result_handle_event (InputEvent);

#endif /* SCREEN_RESULT_H_ */