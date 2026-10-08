#ifndef SCREEN_HELLO_H_
#define SCREEN_HELLO_H_

#include "input_events.h"
#include "screens.h"

void screen_hello_on_enter (void);
void screen_hello_render (void);
ScreenID screen_hello_handle_event (InputEvent);
void screen_hello_on_exit (void);


#endif /* SCREEN_HELLO_H_ */