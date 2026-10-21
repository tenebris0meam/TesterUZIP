#ifndef SCREEN_TESTCONFIRM_H_
#define SCREEN_TESTCONFIRM_H_

#include "input_events.h"
#include "screens.h"

void screen_testconfirm_on_enter (void);
void screen_testconfirm_on_exit (void);
void screen_testconfirm_render (void);
ScreenID screen_testconfirm_handle_event (InputEvent);

#endif /* SCREEN_TESTCONFIRM_H_ */