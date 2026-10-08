#include "states/idle_setting.h"
#include "input_events.h"
#include "model.h"
#include "display.h"
#include "resolver.h"

void proccessor_idle_on_enter (void) {
    return; 
}

void proccessor_idle_on_exit (void) {
    /*
     * Выйти из Idle можно только на запуск прогона, поэтому результаты
     * прошлого прогона обнуляем здесь - и счётчики, и указатель на
     * очередную последовательность.
     */
    model_reset_results();
    generation_step = 0;

    // Выключаем ввод
    inputevent_deny_input();

    uint8_t TVS_num = test_results.current_device->num_seqs_type0;
    uint8_t VAR_num = test_results.current_device->num_seqs_type1;
    uint8_t GDT_num = test_results.current_device->num_seqs_type2;
    if (TVS_num) {
        resolver_set_TVS_VAR();
    } else if (VAR_num) {
        resolver_set_TVS_VAR();
    } else if (GDT_num) {
        resolver_set_GDT();
    }
    return;
}

void proccessor_idle_proccess (void) {
    return;
}

State proccessor_idle_handle_event (Event event) {
    switch (event) {
        case EnterAccepted: 
            if (display_get_current_screen() == ScreenRunningTest) {
                return Commutation;
            }
            return Idle_Setting;
        case AlertActive: return HardwareFail;
        default: return Idle_Setting;
    }
}