#include "states/idle_setting.h"
#include "model.h"
#include "display.h"
#include "params.h"
#include "resolver.h"
#include "ina239.h"
#include "ina239_user.h"

void proccessor_idle_on_enter (void) {
    /* Restore the normal overcurrent threshold after leaving manual generation. */
    (void)INA239_WriteShuntOvervoltageLimit(INA239_GetInstance(), (int16_t)SOVL_RAW);
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
    resolver_set_new_mode(test_config.mode_id);
    if (test_config.mode_id == CheckDevice) {
        uint8_t TVS_num = test_results.current_device->num_seqs_type0;
        uint8_t VAR_num = test_results.current_device->num_seqs_type1;
        uint8_t GDT_num = test_results.current_device->num_seqs_type2;
        if (TVS_num) {
            resolver_set_TVS_VAR();
        } else if (GDT_num) {
            resolver_set_GDT();
        } else if (VAR_num) {
            resolver_set_TVS_VAR();
        }
    }
    return;
}

void proccessor_idle_proccess (void) {
    return;
}

State proccessor_idle_handle_event (Event event) {
    switch (event) {
        case EnterAccepted:
            switch (display_get_current_screen()) {
                case ScreenRunningTest:
                case ScreenGenerator:
                    return Commutation;
                default:
                    return Idle_Setting;
            }
        case ReturnToIdle: return Idle_Setting;
        case AlertActive: return HardwareFail;
        default: return Idle_Setting;
    }
}