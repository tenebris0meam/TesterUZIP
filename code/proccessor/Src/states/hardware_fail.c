#include "states/hardware_fail.h"
#include "calc.h"
#include "display.h"
#include "buzzer.h"
#include "ina239_user.h"
#include "delay.h"
#include "main.h"
#include "stm32f1xx_ll_tim.h"

void proccessor_hardwarefail_on_enter (void) {
    /*
     * Сюда попадаем и по AlertActive из TIM1_BRK, и из состояний, где
     * генерации нет. Поэтому обесточиваем всё безусловно, не полагаясь на
     * то, что это уже сделал обработчик прерывания.
     */
    proccessor_shutdown_generation();
    commutator_forcedeny_commutation();
    calc_reset_integrator();
    display_allow_render();
    inputevent_allow_input();

    buzzer_turn_on();
    display_set_screen(ScreenHardwareFail);

	INA239_Handle_t *hina = INA239_GetInstance();
	uint16_t cur_diag_alert = 0;
    uint8_t delay_id = 0;
    delay_register_task(&delay_id, 3000, Millis);
    while (INA239_ReadDiagAlert(hina, &cur_diag_alert) != INA239_OK) {
        if (!delay_is_not_yet_expired(delay_id)) {
            return;
        }
    }
    delay_unregister_task(delay_id);
    
    /* После чтения регистра DIAG все источники ALERT очищаются.
       Пин ALARM поднимется, BKIN можно очищать */
    LL_TIM_ClearFlag_BRK(TIM1);

    /* Запрос на прерывание уже улетел в NVIC
       Очищаем очередь ожидания */
    NVIC_ClearPendingIRQ(TIM1_BRK_IRQn);

    if (cur_diag_alert & INA239_DIAG_SHNTUL_Msk) {
        /** Ток пошёл не в ту сторону */
        typefail |= UnderCurrent;
    }
    if (cur_diag_alert & INA239_DIAG_BUSUL_Msk) {
        /** Не та полярность напряжения */
        typefail |= UnderVoltage;
    }
    if (cur_diag_alert & INA239_DIAG_SHNTOL_Msk) {
        /** Превышение хардварного порога по току */
        typefail |= OverCurrent;
    }
    if (cur_diag_alert & INA239_DIAG_BUSOL_Msk) {
        /** Превышение хардварного порога по напряжению */
        typefail |= OverVoltage;
    }
    if (cur_diag_alert & INA239_DIAG_MATHOF_Msk) {
        /** Математическая ошибка */
        typefail |= MathOverflow;
    }
    if (!(cur_diag_alert & INA239_DIAG_MEMSTAT_Msk)) {
        /** Ошибка памяти INA239 */
        typefail |= MemoryError;
    }
    return;
}

void proccessor_hardwarefail_on_exit (void) {
    if ((typefail & MathOverflow) || (typefail & MemoryError)) {
        if (!INA239_MainInit()) {
            Error_Handler();
        }
        typefail &= ~(MathOverflow | MemoryError); 
    }
    if (typefail & OverCurrent) {
        typefail &= ~(OverCurrent);
    }
    if (typefail & OverVoltage) {
        typefail &= ~(OverVoltage);
    }
    if (typefail & UnderCurrent) {
        typefail &= ~(UnderCurrent);
    }
    if (typefail & UnderVoltage) {
        typefail &= ~(UnderVoltage);
    }
    current_event = Event_Count;
    return;
}

void proccessor_hardwarefail_proccess (void) {
    return;
}

State proccessor_hardwarefail_handle_event (Event event) {
    switch (event) {
        case EnterAccepted: return Idle_Setting;
        default: return HardwareFail;
    }
}