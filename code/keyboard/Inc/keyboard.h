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

extern volatile uint8_t buttons_flag;

bool keyboard_init (void);
void keyboard_routine (void);
Keyboard_ActiveButton keyboard_get_active_button (void);
bool keyboard_is_button_pressed (void);

#endif /* KEYBOARD_H_ */