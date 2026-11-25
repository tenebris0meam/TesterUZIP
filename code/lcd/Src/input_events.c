#include "input_events.h"
#include "delay.h"
#include "display.h"
#include "keyboard.h"
#include "screens.h"

#include <stdint.h>
#include <stdbool.h>

/** @brief Сколько держится экран приветствия, мс. */
#define INPUTEVENT_HELLO_MS (2000)

/** @brief Сколько держится экран отмены теста, мс */
#define TESTDENIED_MS (2000)

static InputEvent cur_display_state = InputEvent_Count;
static bool is_init = false;
static bool is_test_denied = false;
static uint8_t init_task_id = 0;
static uint8_t testdenied_task_id = 0;
static bool is_testing_complete = false;

/** @brief Кнопка, по которой уже выдано событие - нужна для детекта фронта. */
static Keyboard_ActiveButton last_button = NoActive;



void inputevent_enable_init (void) {
    is_init = true;
    delay_register_task(&init_task_id, INPUTEVENT_HELLO_MS, Millis);
    return;
}

void inputevent_enable_testdenied (void) {
    is_test_denied = true;
    delay_register_task(&testdenied_task_id, TESTDENIED_MS, Millis);
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

void inputevent_enable_systemevent (void) {
    cur_display_state = InputEvent_SystemEvent;
    return;
}

void inputevent_enable_hardwarefail (void) {
    cur_display_state = InputEvent_HardwareFail;
    return;
}

void inputevent_routine (void) {
    if (cur_display_state == InputEvent_Count) {
        /* Завершение прогона важнее всего остального */
        if (is_testing_complete) {
            is_testing_complete = false;
            cur_display_state = InputEvent_TestSequenceComplete;
            return;
        }

        if (is_init) {
            if (!delay_is_not_yet_expired(init_task_id)) {
                is_init = false;
                cur_display_state = InputEvent_HelloScreenTimeout;
            }
            return;
        }

        if (is_test_denied) {
            if (!delay_is_not_yet_expired(testdenied_task_id)) {
                is_test_denied = false;
                cur_display_state = InputEvent_TestDeniedScreenTimeout;
            }
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
        static uint8_t cancel_pressed_num = 0;
        static uint8_t cancel_tripple_task_id = 0;
        switch (button) {
            case PreviousActive: cur_display_state = InputEvent_ButtonPressed_Previous; break;
            case CancelActive: {
                ScreenID cur_screen = display_get_current_screen();
                if ((cur_screen == ScreenTestRunning) || 
                    (cur_screen == ScreenMultiPoleWait) || 
                    (cur_screen == ScreenMultiPoleCountdown)) {
                    if (!cancel_pressed_num) {
                        delay_register_task(&cancel_tripple_task_id, 1000, Millis);
                        cancel_pressed_num++;
                        cur_display_state = InputEvent_ButtonPressed_Cancel;
                    } else if (delay_is_not_yet_expired(cancel_tripple_task_id)) {
                        delay_update_task(cancel_tripple_task_id);
                        cancel_pressed_num++;
                        if (cancel_pressed_num == 3) {
                            delay_unregister_task(cancel_tripple_task_id);
                            cancel_pressed_num = 0;
                            cur_display_state = InputEvent_ButtonTripplePressed_Cancel;
                        } else {
                            cur_display_state = InputEvent_ButtonPressed_Cancel;
                        }
                    } else {
                        cancel_pressed_num = 0;
                        cur_display_state = InputEvent_ButtonPressed_Cancel;
                    }
                } else {
                    cur_display_state = InputEvent_ButtonPressed_Cancel; 
                }
                break;
            }
            case NextActive: cur_display_state = InputEvent_ButtonPressed_Next; break;
            case EnterActive: cur_display_state = InputEvent_ButtonPressed_Enter; break;
            default: break;
        }
    }
    return;
}
