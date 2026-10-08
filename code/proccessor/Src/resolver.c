#include "resolver.h"
#include "commutator.h"
#include "hv_feedback.h"
#include "model.h"
#include "params.h"
#include "proccessor.h"
#include "calc.h"
#include "tim.h"
#include "stm32f1xx_ll_tim.h"
#include "ina239_user.h"
#include "delay.h"
#include "main.h"
#include "states/generation.h"
#include "states/hardware_fail.h"

#include <stddef.h>

#define MAX_TESTS_FOR_ONE_GDT 5

/** @brief Переменные для хранения порогов напряжения устройства */
int32_t voltage_lowth_uV = 0;
int32_t voltage_highth_uV = 0;


int32_t voltage_max_uV = 0;
int32_t voltage_max_buf[MAX_TESTS_FOR_ONE_GDT] = { 0 };
const uint8_t num_tests_for_one_GDT = MAX_TESTS_FOR_ONE_GDT;
uint8_t test_id = 0;

static void resolve_generation_proccess_for_GDT (void) {
    if (have_new_data) {

        have_new_data = false;
        calc_routine();

        int32_t duty_ppm = calc_integrator_get_integral_value();
        if (duty_ppm < 0) {
            duty_ppm = 0;
        }
        if (duty_ppm > CALC_DUTY_MAX_PPM) {
            duty_ppm = CALC_DUTY_MAX_PPM;
        }

        uint32_t new_CCR1_val = (uint32_t)(((uint64_t)LL_TIM_GetAutoReload(TIM1) * 
                                            (uint64_t)duty_ppm) / 1000000ULL);
        uint32_t cur_CCR1_val = LL_TIM_OC_GetCompareCH1(TIM1);
        if (new_CCR1_val != cur_CCR1_val) {
            TIM1_set_CCR_value(new_CCR1_val);
        }

        int32_t current_nA, voltage_uV;
        calc_get_test_data(&voltage_uV, &current_nA);
        if (voltage_max_uV < voltage_uV) {
            voltage_max_uV = voltage_uV;
        }

        if (voltage_uV >= voltage_highth_uV) {
            /* Напряжение генерации стало больше напряжения ионизации газа, поломка */
            proccessor_raise_event(DeviceVoltageOverVoltage);
        } else if (voltage_uV < voltage_lowth_uV) {
            /* Напряжение генерации не достигло напряжения ионизации газа, ждём ещё */
            proccessor_raise_event(DeviceVoltageUnderVoltage);
        }
    }
    return;
}

static State resolve_generation_event_for_GDT (Event event) {
    switch (event) {
        case DeviceVoltageUnderVoltage: 
            if (calc_is_voltage_achieved()) {
                generation_step++;
                if (!proccessor_apply_setpoint()) {
                    return TestFailed;
                }
            }
            return Generation;

        case DeviceVoltageOverVoltage: return TestFailed;

        case AlertActive: return HardwareFail;

        default: return Generation;
    }
}

static void resolve_hardwarefail_enter_for_GDT (void) {
    /*
     * Сюда попадаем только из прерывания по TIM1 BKIN
     * Поэтому считаем, что коммутация уже отключена
     * Нужно убрать источник BKIN, реинициализировать INA239
     */

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

static void resolve_hardwarefail_exit_for_GDT (void) {
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

static void resolve_hardwarefail_proccess_for_GDT (void) {
    if (test_id >= num_tests_for_one_GDT) {
        TestResult_TypeDef result = { 0 };
        result.sequence_pos = test_results.failed_cnt + test_results.passed_cnt;
        for (uint8_t i = 0; i < num_tests_for_one_GDT; i++) {
            result.device_voltage += voltage_max_buf[i];
        }
        result.device_voltage /= num_tests_for_one_GDT;

        int64_t dispersion_voltage_uV = 0;
        for (uint8_t i = 0; i < num_tests_for_one_GDT; i++) {
            int32_t delta_voltage_uV = voltage_max_buf[i] - result.device_voltage;
            int64_t delta_voltage_uV_sqr = (int64_t)delta_voltage_uV * (int64_t)delta_voltage_uV;
            dispersion_voltage_uV += delta_voltage_uV_sqr;
        }
        dispersion_voltage_uV /= num_tests_for_one_GDT - 1;

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
        Commutator_ActiveLine positive = (Commutator_ActiveLine)(test_results.current_sequence->input_pin + 1);
        Commutator_ActiveLine negative = (Commutator_ActiveLine)(test_results.current_sequence->output_pin + 1);

        commutator_set_active_interface(test_config.interface_id);
        commutator_set_active_line(positive, LineType_Positive);
        commutator_set_active_line(negative, LineType_Negative);

        commutator_allow_commutation();

        proccessor_raise_event(DeviceCurrentWithinTheRange);
        return;
    }
}

static State resolve_hardwarefail_event_for_GDT (Event event) {
    switch (event) {
        case TestSequenceFinished: return TestSequenceComplete;
        case TestResultRecorded: return Commutation;
        default: return Generation;
    }
}

/* -------------------------------------------------------------------- */
/* ----------------------- Public Functions --------------------------- */
/* -------------------------------------------------------------------- */

void resolver_set_GDT (void) {
    proccessor[Generation].proccess = resolve_generation_proccess_for_GDT;
    proccessor[Generation].handle_event = resolve_generation_event_for_GDT;
    proccessor[HardwareFail].on_enter = resolve_hardwarefail_enter_for_GDT;
    proccessor[HardwareFail].proccess = resolve_hardwarefail_proccess_for_GDT;
    proccessor[HardwareFail].on_exit = resolve_hardwarefail_exit_for_GDT;
    proccessor[HardwareFail].handle_event = resolve_hardwarefail_event_for_GDT;
    return;
}

void resolver_set_TVS_VAR (void) {
    proccessor[Generation].proccess = proccessor_generation_proccess;
    proccessor[Generation].handle_event = proccessor_generation_handle_event;
    proccessor[HardwareFail].on_enter = proccessor_hardwarefail_on_enter;
    proccessor[HardwareFail].proccess = proccessor_hardwarefail_proccess;
    proccessor[HardwareFail].on_exit = proccessor_hardwarefail_on_exit;
    proccessor[HardwareFail].handle_event = proccessor_hardwarefail_handle_event;
    return;
}

void resolver_fill_thresholds_with_current_testparams (void) {
    const Params_TestParam_TypeDef *testparams = test_results.current_test_params;

    voltage_highth_uV = hv_feedback_volts_to_target_uV(testparams->max_voltage);
    voltage_lowth_uV = hv_feedback_volts_to_target_uV(testparams->min_voltage);

    return;
}