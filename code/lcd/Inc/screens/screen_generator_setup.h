#ifndef SCREEN_GENERATOR_SETUP_H_
#define SCREEN_GENERATOR_SETUP_H_
#include "screens.h"
void screen_generatorsetup_on_enter(void);
void screen_generatorsetup_on_exit(void);
void screen_generatorsetup_render(void);
ScreenID screen_generatorsetup_handle_event(InputEvent event);
#endif
