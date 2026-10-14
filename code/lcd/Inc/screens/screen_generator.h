#ifndef SCREEN_GENERATOR_H_
#define SCREEN_GENERATOR_H_
#include "screens.h"
void screen_generator_on_enter(void);
void screen_generator_on_exit(void);
void screen_generator_render(void);
ScreenID screen_generator_handle_event(InputEvent event);
#endif
