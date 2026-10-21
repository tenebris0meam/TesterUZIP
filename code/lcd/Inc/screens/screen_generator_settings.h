#ifndef SCREEN_GENERATOR_SETTINGS_H_
#define SCREEN_GENERATOR_SETTINGS_H_

#include "screens.h"

void screen_generator_settings_on_enter(void);
void screen_generator_settings_on_exit(void);
void screen_generator_settings_render(void);
ScreenID screen_generator_settings_handle_event(InputEvent event);

#endif /* SCREEN_GENERATOR_SETTINGS_H_ */