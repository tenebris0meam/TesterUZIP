#include "buzzer.h"
#include "delay.h"
#include "keyboard.h"
#include "gpio.h"



const Buzzer_TypeDef buzzer = {
    .pin = &BUZZER,
    .beep_time_ms = 100,
};

static volatile bool beep_in_operation = false;
static volatile uint8_t delay_task_id = 0;

void buzzer_turn_on (void) {
    if (beep_in_operation) {
        return;
    } else {
        if (delay_register_task((uint8_t *)&delay_task_id, buzzer.beep_time_ms, Millis)) {
            beep_in_operation = true;
            Pin_Set_State((Pin_TypeDef *)buzzer.pin, false);
        } 
    }
    return;
}

void buzzer_turn_off (void) {
    Pin_Set_State((Pin_TypeDef *)buzzer.pin, true);
    beep_in_operation = false;
    return;
}

bool buzzer_is_beep_in_operation (void) {
    return beep_in_operation;
}

void buzzer_routine (void) {
    if (beep_in_operation) {
        if (!delay_is_not_yet_expired(delay_task_id)) {
            buzzer_turn_off();
        }
    } else {
        if (keyboard_is_button_pressed()) {
            buzzer_turn_on();
        }
    }
    return;
}