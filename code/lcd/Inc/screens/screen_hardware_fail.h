#ifndef SCREEN_HARDWARE_FAIL_H_
#define SCREEN_HARDWARE_FAIL_H_

#include "input_events.h"
#include "screens.h"

void screen_hardwarefail_on_enter (void);
void screen_hardwarefail_on_exit (void);
void screen_hardwarefail_render (void);
ScreenID screen_hardwarefail_handle_event (InputEvent);

#endif /* SCREEN_HARDWARE_FAIL_H_ */