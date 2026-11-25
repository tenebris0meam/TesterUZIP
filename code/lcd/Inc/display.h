#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "screens.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DisplayUpdateEnable,
    DisplayUpdateSuspended
} DisplayUpdatePolicy;

bool display_init (void);
ScreenID display_get_current_screen (void);
void display_routine (void);
void display_set_updatepolicy (const DisplayUpdatePolicy new_policy);

#endif /* DISPLAY_H_ */