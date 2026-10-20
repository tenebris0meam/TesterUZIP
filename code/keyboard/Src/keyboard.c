#include "keyboard.h"
#include "exti.h"
#include <stdbool.h>
#include <stdint.h>

static Keyboard_ActiveButton active_button = NoActive;
static bool button_pressed = false;

bool keyboard_init (void) {
    EXTI_enable_interrupt();
    return true;
}

void keyboard_set_active_button (Keyboard_ButtonNames button_flag) {
    switch (button_flag) {
        case Previous:
            active_button = PreviousActive;
            button_pressed = true;
            break;
        case Cancel:
            active_button = CancelActive;
            button_pressed = true;
            break;
        case Next: 
            active_button = NextActive;
            button_pressed = true;
            break;
        case Enter: 
            active_button = EnterActive;
            button_pressed = true;
            break;
        default: 
            active_button = NoActive;
            button_pressed = false;
            break;
    }
    return;
}

Keyboard_ActiveButton keyboard_get_active_button (void) {
    return active_button;
}

bool keyboard_is_button_pressed (void) {
    return button_pressed;
}