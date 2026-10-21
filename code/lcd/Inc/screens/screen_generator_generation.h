#ifndef SCREEN_GENERATOR_GENERATION_H_
#define SCREEN_GENERATOR_GENERATION_H_

#include "screens.h"

void screen_generator_generation_on_enter(void);
void screen_generator_generation_on_exit(void);
void screen_generator_generation_render(void);
ScreenID screen_generator_generation_handle_event(InputEvent event);

#endif /* SCREEN_GENERATOR_GENERATION_H_ */