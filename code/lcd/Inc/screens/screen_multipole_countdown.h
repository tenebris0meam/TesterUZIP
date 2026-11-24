#ifndef SCREEN_MULTIPOLE_COUNTDOWN_H_
#define SCREEN_MULTIPOLE_COUNTDOWN_H_

#include "screens.h"

void screen_multipole_countdown_on_enter(void);
void screen_multipole_countdown_on_exit(void);
void screen_multipole_countdown_render(void);
ScreenID screen_multipole_countdown_handle_event(InputEvent event);

#endif /* SCREEN_MULTIPOLE_COUNTDOWN_H_ */
