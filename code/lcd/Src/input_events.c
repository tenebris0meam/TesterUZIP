#include "input_events.h"
#include "delay.h"
#include "keyboard.h"

#include <stdint.h>
#include <stdbool.h>

/** @brief Сколько держится экран приветствия, мс. */
#define INPUTEVENT_HELLO_MS (3000)

static InputEvent cur_display_state = InputEvent_Count;
static bool is_init = false;
static uint8_t init_task_id = 0;
static bool is_input_allowed = false;
static bool is_testing_complete = false;

/** @brief Кнопка, по которой уже выдано событие - нужна для детекта фронта. */
static Keyboard_ActiveButton last_button = NoActive;



void inputevent_enable_init (void) {
    is_init = true;
    delay_register_task(&init_task_id, INPUTEVENT_HELLO_MS, Millis);
    return;
}

void inputevent_allow_input (void) {
    is_input_allowed = true;
    return;
}

void inputevent_deny_input (void) {
    is_input_allowed = false;
    return;
}

void inputevent_testing_completed (void) {
    is_testing_complete = true;
    return;
}

InputEvent inputevent_get_state (void) {
    InputEvent cur_state = cur_display_state;
    cur_display_state = InputEvent_Count;
    return cur_state;
}

void inputevent_routine (void) {
    cur_display_state = InputEvent_Count;

    /* Завершение прогона важнее всего остального */
    if (is_testing_complete) {
        is_testing_complete = false;
        cur_display_state = InputEvent_TestSequenceComplete;
        return;
    }

    /* Пока висит экран приветствия, ввод не принимаем */
    if (is_init) {
        if (!delay_is_not_yet_expired(init_task_id)) {
            is_init = false;
            is_input_allowed = true;
            cur_display_state = InputEvent_HelloScreenTimeout;
        }
        return;
    }

    if (!is_input_allowed) {
        return;
    }

    /*
     * Событие выдаём по фронту. keyboard_is_button_pressed() держится весь
     * период антидребезга (300 мс), а главный цикл за это время проворачивается
     * сотни раз - по уровню одно нажатие уводило бы курсор через всё меню.
     */
    Keyboard_ActiveButton button = keyboard_is_button_pressed() ? 
                                   keyboard_get_active_button() : 
                                   NoActive;
    if (button == last_button) {
        return;
    }
    last_button = button;

    switch (button) {
        case PreviousActive: cur_display_state = InputEvent_ButtonPressed_Previous; break;
        case CancelActive: cur_display_state = InputEvent_ButtonPressed_Cancel; break;
        case NextActive: cur_display_state = InputEvent_ButtonPressed_Next; break;
        case EnterActive: cur_display_state = InputEvent_ButtonPressed_Enter; break;
        default: break;
    }
    return;
}
