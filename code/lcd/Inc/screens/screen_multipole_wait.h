#ifndef SCREEN_MULTIPOLE_WAIT_H_
#define SCREEN_MULTIPOLE_WAIT_H_

#include "screens.h"

void screen_multipole_wait_on_enter(void);
void screen_multipole_wait_on_exit(void);
void screen_multipole_wait_render(void);
ScreenID screen_multipole_wait_handle_event(InputEvent event);

#endif /* SCREEN_MULTIPOLE_WAIT_H_ */
