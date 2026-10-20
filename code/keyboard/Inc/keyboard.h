#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    Previous,
    Cancel,
    Next,
    Enter
} Keyboard_ButtonNames;

typedef enum {
    NoActive,
    PreviousActive,
    CancelActive,
    NextActive,
    EnterActive
} Keyboard_ActiveButton;

bool keyboard_init (void);
Keyboard_ActiveButton keyboard_get_active_button (void);
bool keyboard_is_button_pressed (void);
void keyboard_set_active_button (Keyboard_ButtonNames button_flag);

#endif /* KEYBOARD_H_ */