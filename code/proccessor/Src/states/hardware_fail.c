#include "states/hardware_fail.h"
#include "calc.h"
#include "display.h"
#include "buzzer.h"
#include "ina239.h"
#include "ina239_user.h"
#include "delay.h"
#include "main.h"
#include "proccessor.h"
#include "resolver.h"
#include "stm32f1xx_ll_tim.h"
#include "model.h"
#include "states/generation.h"
#include "tim.h"
#include "utils.h"
#include <stdint.h>

void proccessor_hardwarefail_on_enter_default (void) {
    /*
     * Сюда попадаем и по AlertActive из TIM1_BRK, и из состояний, где
     * генерации нет. Поэтому обесточиваем всё безусловно, не полагаясь на
     * то, что это уже сделал обработчик прерывания.
     */
    proccessor_shutdown_generation();
    commutator_forcedeny_commutation();
    calc_integrator_reset();
    display_allow_render();

    buzzer_turn_on();

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
    display_set_screen(ScreenHardwareFail);
    return;
}

void proccessor_hardwarefail_on_exit_default (void) {
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

void proccessor_hardwarefail_proccess_default (void) {
    return;
}

State proccessor_hardwarefail_handle_event_default (Event event) {
    switch (event) {
        case EnterAccepted: return Idle_Setting;
        default: return HardwareFail;
    }
}





void proccessor_hardwarefail_on_enter_for_GDT (void) {
    /*
     * Сюда попадаем только из прерывания по TIM1 BKIN
     * Поэтому считаем, что коммутация уже отключена
     * Нужно убрать источник BKIN, реинициализировать INA239
     */
    proccessor_force_shutdown_generation();
    TIM3_Stop_Count();

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

    if ((typefail & MathOverflow) || (typefail & OverCurrent)) {
        if (!INA239_MainInit()) {
            Error_Handler();
        }
        if (INA239_SetMode(INA239_MODE_CONT_SHUNT_BUS) != INA239_OK) {
            Error_Handler();
        }
        TIM3_Start_Count();
        proccessor_wait_cap_discharge();
        TIM3_Stop_Count();
        (void)INA239_SetMode(INA239_MODE_SHUTDOWN);
        typefail &= ~(MathOverflow | OverCurrent); 
        // Если была ошибка переполнения, то засчитываем успешный тест
        // АЦП INA239 зашкалило от броска тока 
        // и в регистр VSHNT попало переполненное значение

        voltage_max_buf[test_id] = voltage_max_uV;
        test_id++;
    }
    voltage_max_uV = 0;

    if (typefail) {
        // непредвиденная ошибка, значит что-то случилось
        // передаём в resolver_set_resolve_type невалидное значение
        // чтобы указатели на функции в proccessor вернулись
        // к состоянию сразу после инициализации
        resolver_set_TVS_VAR();

        // и ещё раз поднимаем AlertActive
        proccessor_force_raise_event(AlertActive);
    }

    return;
}

void proccessor_hardwarefail_on_exit_for_GDT (void) {
    return;
}



static inline int32_t get_average_from_buf (int32_t *buf, uint32_t num_el_buf) {
    int64_t acc = 0;
    for (uint32_t i = 0; i < num_el_buf; i++) {
        acc += (int64_t)buf[i];
    }
    acc /= (int64_t)num_el_buf;
    return (int32_t)acc;
}

static inline int32_t get_standard_deviation_from_buf (int32_t *buf, uint32_t num_el_buf, int32_t mean_val) {
    int64_t acc = 0;
    for (uint32_t i = 0; i < num_el_buf; i++) {
        int64_t cur_delta = (int64_t)buf[i] - (int64_t)mean_val;
        acc += cur_delta * cur_delta;
    }
    acc /= (int64_t)(num_el_buf - 1);
    return (int32_t)uint64_sqrt(acc);
}

void proccessor_hardwarefail_proccess_for_GDT (void) {
    if (test_id >= MAX_TESTS_FOR_ONE_GDT) {
        uint8_t pos = test_runtime.failed_cnt + test_runtime.passed_cnt;
        test_results[pos].sequence_pos = pos;
        test_results[pos].type = (uint8_t)GasDischargeTube;
        test_results[pos].res.voltage_uV = 
            get_average_from_buf(voltage_max_buf, 
                                 MAX_TESTS_FOR_ONE_GDT);
        test_results[pos].res.current_nA = INT32_MAX;
        model_handle_test_result();
        proccessor_raise_after_result();
        test_id = 0;
        return;
    } else {
        proccessor_raise_event(DeviceCurrentWithinTheRange);
        return;
    }
}

State proccessor_hardwarefail_handle_event_for_GDT (Event event) {
    switch (event) {
        case TestSequenceFinished: return TestSequenceComplete;
        case DeviceCurrentWithinTheRange: 
        case TestResultRecorded: return Commutation;
        default: return HardwareFail;
    }
}