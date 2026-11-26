#include "keyboard.h"
#include "gpio.h"
#include "stm32f1xx_ll_gpio.h"
#include "system_stm32f1xx.h"
#include "tim.h"
#include <stdbool.h>
#include <stdint.h>

#define KEYBOARD_DEBOUNCE_MS (50)
#define KEYBOARD_COOLDOWN_MS (50)
#define STABLE_SAMPLES (KEYBOARD_DEBOUNCE_MS / POLL_PERIOD_MS)

/** @brief Состояние одной кнопки при антидребезге. */
typedef struct {
    const Pin_TypeDef *button;  /**< Пин кнопки */
    Keyboard_ButtonNames name;  /**< Логическое имя кнопки */
    uint8_t cnt;  /**< Счётчик подряд идущих отсчётов нового состояния */
    bool is_pressed;  /**< Подтверждённое состояние: нажата */
    bool is_active;  /**< Событие по этому нажатию уже выдано */
} button_t;

static button_t buttons[Button_Count] = {
    { .button = &BUTTON_UP, .name = Previous, .cnt = 0, .is_pressed = false, .is_active = false }, 
    { .button = &BUTTON_REJECT, .name = Cancel, .cnt = 0, .is_pressed = false, .is_active = false }, 
    { .button = &BUTTON_DOWN, .name = Next, .cnt = 0, .is_pressed = false, .is_active = false }, 
    { .button = &BUTTON_ACCEPT, .name = Enter, .cnt = 0, .is_pressed = false, .is_active = false }
};

static Keyboard_ActiveButton active_button = NoActive;
static bool button_pressed = false;
static uint32_t last_event_ms = 0;
static bool event_active = false;

void keyboard_init (void) {
    for (uint8_t i = 0; i < Button_Count; i++) {
        buttons[i].cnt = 0;
        buttons[i].is_pressed = false;
        buttons[i].is_active = false;
    }
    active_button = NoActive;
    button_pressed = false;
    last_event_ms = 0;
    event_active = false;
    TIM4_Start();
    return;
}

void keyboard_routine (void) {
    uint32_t cur_time_ms = DWT->CYCCNT / (SystemCoreClock / 1000);

    if (event_active && ((cur_time_ms - last_event_ms) >= KEYBOARD_COOLDOWN_MS)) {
        event_active = false;
        button_pressed = false;
    }
    for (uint8_t i = 0; i < Button_Count; i++) {
        button_t *b = &buttons[i];
        bool raw = !LL_GPIO_IsInputPinSet(b->button->port, b->button->pin);
        if (raw == b->is_pressed) {
            b->cnt = 0;
            continue;
        }

        if (++b->cnt < STABLE_SAMPLES) {
            continue;
        }

        b->cnt = 0;
        b->is_pressed = raw;

        if (raw) {
            if (!b->is_active) {
                if (!event_active) {
                    active_button = (Keyboard_ActiveButton)(i + 1);
                    button_pressed = true;
                    event_active = true;
                    last_event_ms = cur_time_ms;
                }
                b->is_active = true;
            }
        } else {
            if (b->is_active) {
                b->is_active = false;
            }
        }
    }
}

Keyboard_ActiveButton keyboard_get_active_button (void) {
    return active_button;
}

bool keyboard_is_button_pressed (void) {
    return button_pressed;
}