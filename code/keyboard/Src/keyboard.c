#include "adc.h"
#include "keyboard.h"
#include "window.h"
#include "delay.h"

#include <stdint.h>



static Keyboard_ActiveButton active_button = NoActive;
static bool button_pressed = false;



static Keyboard_ActiveButton get_active_button (Window_TypeDef *window) {
    Keyboard_ActiveButton act = NoActive;
#if defined DEBUG
    for (Keyboard_ButtonNames button = Previous; button <= Beep; button++) 
#else
    for (Keyboard_ButtonNames button = Previous; button <= Enter; button++)
#endif
    {
        uint32_t cnt = 0;
        for (uint32_t i = 0; i < window->num_elements; i++) {
            if ((Keyboard_ActiveButton)*(uint8_t *)(window->buf + i) == NoActive) {
                return NoActive;
            }
            cnt = ((Keyboard_ActiveButton)(button + 1) == (Keyboard_ActiveButton)*(uint8_t *)(window->buf + i) ? cnt + 1 : cnt);
        }
        if (cnt == window->num_elements) {
            act = (Keyboard_ActiveButton)(button + 1);
            break;
        }
    }
    return act;
}

void keyboard_init (void) {
    ADC1_Start();
    return;
}

void keyboard_routine (void) {
    static Keyboard_ActiveButton prev_button = NoActive;
    static Keyboard_ActiveButton debounce_button = NoActive;
    static bool debounce_active = false;
    static bool cooling_active = false;
    static uint8_t debounce_task_id = 0;
    static uint8_t cooling_task_id = 0;

    const int32_t delay_debounce_ms = 100;
    const int32_t delay_cooling_ms = 100;

    uint32_t cur_keyboard_voltage = ADC1_get_keyboard_voltage_uV();
    Keyboard_ActiveButton cur_button = NoActive;

    if ((cur_keyboard_voltage >= BUTTON1_LOWTH_uV) && (cur_keyboard_voltage < BUTTON1_HIGHTH_uV)) {
        cur_button = PreviousActive;
    } else if ((cur_keyboard_voltage >= BUTTON2_LOWTH_uV) && (cur_keyboard_voltage < BUTTON2_HIGHTH_uV)) {
        cur_button = CancelActive;
    } else if ((cur_keyboard_voltage >= BUTTON3_LOWTH_uV) && (cur_keyboard_voltage < BUTTON3_HIGHTH_uV)) {
        cur_button = NextActive;
    } else if ((cur_keyboard_voltage >= BUTTON4_LOWTH_uV) && (cur_keyboard_voltage < BUTTON4_HIGHTH_uV)) {
        cur_button = EnterActive;
    }
    #if defined DEBUG
    else if ((cur_keyboard_voltage >= BUTTON5_LOWTH_uV) && (cur_keyboard_voltage < BUTTON5_HIGHTH_uV)) {
        cur_button = BeepActive;
    }
    #endif
    else {
        cur_button = NoActive;
    }

    if (!cooling_active) {
        if (!debounce_active) {
            if (cur_button != NoActive) {
                if (delay_register_task(&debounce_task_id, delay_debounce_ms, Millis)) {
                    debounce_active = true;
                    debounce_button = cur_button;
                } else {
                    return;
                }
            }
        } else if (!delay_is_not_yet_expired(debounce_task_id)) {
            debounce_active = false;
            if (debounce_button == cur_button) {
                if (delay_register_task(&cooling_task_id, delay_cooling_ms, Millis)) {
                    prev_button = active_button;
                    active_button = debounce_button;
                    button_pressed = true;
                    cooling_active = true;
                }
            } else {
                debounce_button = NoActive;
            }
        }
    } else if (!delay_is_not_yet_expired(cooling_task_id)) {
        cooling_active = false;
        button_pressed = false;
        prev_button = active_button;
        active_button = NoActive;
    }
    return;
}

Keyboard_ActiveButton keyboard_get_active_button (void) {
    return active_button;
}

bool keyboard_is_button_pressed (void) {
    return button_pressed;
}