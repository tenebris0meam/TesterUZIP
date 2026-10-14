#include "states/hardware_fail.h"
#include "calc.h"
#include "display.h"
#include "buzzer.h"
#include "ina239.h"
#include "ina239_user.h"
#include "delay.h"
#include "main.h"
#include "proccessor.h"
#include "stm32f1xx_ll_tim.h"
#include "model.h"
#include "states/generation.h"
#include "hv_feedback.h"
#include "tim.h"

void proccessor_hardwarefail_on_enter_default (void) {
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
    // не забыть сбросить значение максимального напряжения
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

static uint64_t uint64_sqrt (uint64_t n) {
    uint64_t res = 0;
    uint64_t bit = 1ULL << 62; // Начинаем со старшей четной степени 2 для 32-битного числа (2^30)

    // Сдвигаем бит до тех пор, пока он больше n
    while (bit > n) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
} 

void proccessor_hardwarefail_proccess_for_GDT (void) {
    if (test_id >= MAX_TESTS_FOR_ONE_GDT) {
        TestResult_TypeDef result = { 0 };
        result.sequence_pos = test_results.failed_cnt + test_results.passed_cnt;
        for (uint8_t i = 0; i < MAX_TESTS_FOR_ONE_GDT; i++) {
            result.device_voltage += voltage_max_buf[i];
        }
        result.device_voltage /= MAX_TESTS_FOR_ONE_GDT;

        int64_t dispersion_voltage_uV = 0;
        for (uint8_t i = 0; i < MAX_TESTS_FOR_ONE_GDT; i++) {
            int32_t delta_voltage_uV = voltage_max_buf[i] - result.device_voltage;
            int64_t delta_voltage_uV_sqr = (int64_t)delta_voltage_uV * (int64_t)delta_voltage_uV;
            dispersion_voltage_uV += delta_voltage_uV_sqr;
        }
        dispersion_voltage_uV /= MAX_TESTS_FOR_ONE_GDT - 1;

        result.standard_deviation_voltage = (uint32_t)uint64_sqrt(dispersion_voltage_uV);
        int32_t ionization_voltage_GDT_lowth = result.device_voltage - 3 * result.standard_deviation_voltage;
        int32_t ionization_voltage_GDT_highth = result.device_voltage + 3 * result.standard_deviation_voltage;
        int32_t ionization_voltage_GDT_lowth_pasp = hv_feedback_volts_to_target_uV(test_results.current_test_params->min_voltage);
        int32_t ionization_voltage_GDT_highth_pasp = hv_feedback_volts_to_target_uV(test_results.current_test_params->max_voltage);
        if ((ionization_voltage_GDT_lowth_pasp < ionization_voltage_GDT_lowth) && 
            (ionization_voltage_GDT_highth_pasp >= ionization_voltage_GDT_highth)) {
            // Если стандартное отклонение напряжения ионизации
            // не превысило пороги по паспорту, то считаем тест 
            // пройденным
            result.pass = true;
        } else {
            // Стандартное отклонение вышло за порог по паспорту
            // считаем разрядник павшим смерью храбрых
            result.pass = false;
        }
        model_handle_test_result(&result);
        if (model_get_total_tests() == (test_results.failed_cnt + test_results.passed_cnt)) {
            proccessor_raise_event(TestSequenceFinished);
        } else {
            proccessor_raise_event(TestResultRecorded);
        }
        test_id = 0;
        return;
    } else {
        proccessor_raise_event(DeviceCurrentWithinTheRange);
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