#include "keyboard.h"
#include "exti.h"
// #include "window.h"
// #include "delay.h"
#include "main.h"
#include "stm32f103xb.h"
// #include "stm32f1xx.h"
#include "stm32f1xx_ll_gpio.h"

#include <stdbool.h>
#include <stdint.h>



static Keyboard_ActiveButton active_button = NoActive;
static Keyboard_ActiveButton debounce_button = NoActive;
static bool button_pressed = false;
static const uint32_t button_debounce_delay_tick = 1500000;
static const uint32_t button_cooling_delay_tick = 1500000;
static uint32_t button_debounce_start_tick = 0;
static uint32_t button_cooling_start_tick = 0;
static bool debounce_active = false;
static bool cooling_active = false;

volatile uint8_t buttons_flag = 0;



static bool is_debounce_expired (void) {
    uint32_t timeout_tick = button_debounce_start_tick + button_debounce_delay_tick;
    uint32_t cur_CYCCNT = DWT->CYCCNT;
    if (cur_CYCCNT < button_debounce_start_tick) {
        if (timeout_tick < button_debounce_start_tick) {
            if ((int32_t)cur_CYCCNT > 0) {
                return (cur_CYCCNT > timeout_tick);
            } else {
                return false;
            }
        } else {
            return true;
        }
    } else {
        return (cur_CYCCNT > timeout_tick);
    }
}

static bool is_cooling_expired (void) {
    uint32_t timeout_tick = button_cooling_start_tick + button_cooling_delay_tick;
    uint32_t cur_CYCCNT = DWT->CYCCNT;
    if (cur_CYCCNT < button_cooling_start_tick) {
        if (timeout_tick < button_cooling_start_tick) {
            if ((int32_t)cur_CYCCNT > 0) {
                return (cur_CYCCNT > timeout_tick);
            } else {
                return false;
            }
        } else {
            return true;
        }
    } else {
        return (cur_CYCCNT > timeout_tick);
    }
}

// static Keyboard_ActiveButton get_active_button (Window_TypeDef *window) {
//     Keyboard_ActiveButton act = NoActive;
// #if defined DEBUG
//     for (Keyboard_ButtonNames button = Previous; button <= Beep; button++) 
// #else
//     for (Keyboard_ButtonNames button = Previous; button <= Enter; button++)
// #endif
//     {
//         uint32_t cnt = 0;
//         for (uint32_t i = 0; i < window->num_elements; i++) {
//             if ((Keyboard_ActiveButton)*(uint8_t *)(window->buf + i) == NoActive) {
//                 return NoActive;
//             }
//             cnt = ((Keyboard_ActiveButton)(button + 1) == (Keyboard_ActiveButton)*(uint8_t *)(window->buf + i) ? cnt + 1 : cnt);
//         }
//         if (cnt == window->num_elements) {
//             act = (Keyboard_ActiveButton)(button + 1);
//             break;
//         }
//     }
//     return act;
// }

bool keyboard_init (void) {
    EXTI_enable_interrupt();
    return true;
}

void keyboard_routine (void) {
    if (buttons_flag & UP) {
        active_button = PreviousActive;
        button_pressed = true;
        buttons_flag = 0;
    } else if (buttons_flag & REJECT) {
        active_button = CancelActive;
        button_pressed = true;
        buttons_flag = 0;
    } else if (buttons_flag & DOWN) {
        active_button = NextActive;
        button_pressed = true;
        buttons_flag = 0;
    } else if (buttons_flag & ACCEPT) {
        active_button = EnterActive;
        button_pressed = true;
        buttons_flag = 0;
    } else {
        button_pressed = false;
        active_button = NoActive;
    }
    return;

    // if (!cooling_active && buttons_flag) {
    //     if (buttons_flag & UP) {
    //         debounce_button = PreviousActive;
    //     } else if (buttons_flag & REJECT) {
    //         debounce_button = CancelActive;
    //     } else if (buttons_flag & DOWN) {
    //         debounce_button = NextActive;
    //     } else if (buttons_flag & ACCEPT) {
    //         debounce_button = EnterActive;
    //     } else {
    //         buttons_flag = 0;
    //         debounce_button = NoActive;
    //         return;
    //     }
    //     buttons_flag = 0;
    //     button_debounce_start_tick = DWT->CYCCNT;
    //     debounce_active = true;
    // } else if (cooling_active/* && is_cooling_expired()*/) {
    //     cooling_active = false;
    //     active_button = NoActive;
    //     button_pressed = false;
    // } else if (debounce_active/* && is_debounce_expired()*/) {
    //     debounce_active = false;
    //     GPIO_TypeDef *port;
    //     uint32_t pin;
    //     switch (debounce_button) {
    //         default: 
    //         case PreviousActive: 
    //             port = BUTTON_UP_GPIO_Port;
    //             pin = BUTTON_UP_GPIO_Pin;
    //             break;
    //         case CancelActive: 
    //             port = BUTTON_REJECT_GPIO_Port;
    //             pin = BUTTON_REJECT_GPIO_Pin;
    //             break;
    //         case NextActive: 
    //             port = BUTTON_DOWN_GPIO_Port;
    //             pin = BUTTON_DOWN_GPIO_Pin;
    //             break;
    //         case EnterActive:
    //             port = BUTTON_ACCEPT_GPIO_Port;
    //             pin = BUTTON_ACCEPT_GPIO_Pin;
    //             break;
    //     }
    //     if (!LL_GPIO_IsInputPinSet(port, pin)) {
    //         active_button = debounce_button;
    //         button_pressed = true;
    //     }
    //     debounce_button = NoActive;
    //     button_cooling_start_tick = DWT->CYCCNT;
    //     cooling_active = true;
    // }
    // return;

    // static Keyboard_ActiveButton prev_button = NoActive;
    // static Keyboard_ActiveButton debounce_button = NoActive;
    // static bool debounce_active = false;
    // static bool cooling_active = false;
    // static uint8_t debounce_task_id = 0;
    // static uint8_t cooling_task_id = 0;

    // const int32_t delay_debounce_ms = 100;
    // const int32_t delay_cooling_ms = 100;

    // uint32_t cur_keyboard_voltage = ADC1_get_keyboard_voltage_uV();
    // Keyboard_ActiveButton cur_button = NoActive;

    // if ((cur_keyboard_voltage >= BUTTON1_LOWTH_uV) && (cur_keyboard_voltage < BUTTON1_HIGHTH_uV)) {
    //     cur_button = PreviousActive;
    // } else if ((cur_keyboard_voltage >= BUTTON2_LOWTH_uV) && (cur_keyboard_voltage < BUTTON2_HIGHTH_uV)) {
    //     cur_button = CancelActive;
    // } else if ((cur_keyboard_voltage >= BUTTON3_LOWTH_uV) && (cur_keyboard_voltage < BUTTON3_HIGHTH_uV)) {
    //     cur_button = NextActive;
    // } else if ((cur_keyboard_voltage >= BUTTON4_LOWTH_uV) && (cur_keyboard_voltage < BUTTON4_HIGHTH_uV)) {
    //     cur_button = EnterActive;
    // }
    // #if defined DEBUG
    // else if ((cur_keyboard_voltage >= BUTTON5_LOWTH_uV) && (cur_keyboard_voltage < BUTTON5_HIGHTH_uV)) {
    //     cur_button = BeepActive;
    // }
    // #endif
    // else {
    //     cur_button = NoActive;
    // }

    // if (!cooling_active) {
    //     if (!debounce_active) {
    //         if (cur_button != NoActive) {
    //             if (delay_register_task(&debounce_task_id, delay_debounce_ms, Millis)) {
    //                 debounce_active = true;
    //                 debounce_button = cur_button;
    //             } else {
    //                 return;
    //             }
    //         }
    //     } else if (!delay_is_not_yet_expired(debounce_task_id)) {
    //         debounce_active = false;
    //         if (debounce_button == cur_button) {
    //             if (delay_register_task(&cooling_task_id, delay_cooling_ms, Millis)) {
    //                 prev_button = active_button;
    //                 active_button = debounce_button;
    //                 button_pressed = true;
    //                 cooling_active = true;
    //             }
    //         } else {
    //             debounce_button = NoActive;
    //         }
    //     }
    // } else if (!delay_is_not_yet_expired(cooling_task_id)) {
    //     cooling_active = false;
    //     button_pressed = false;
    //     prev_button = active_button;
    //     active_button = NoActive;
    // }
    // return;
}

Keyboard_ActiveButton keyboard_get_active_button (void) {
    return active_button;
}

bool keyboard_is_button_pressed (void) {
    return button_pressed;
}