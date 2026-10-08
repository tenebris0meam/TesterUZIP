#include "states/generation.h"
#include "calc.h"
#include "tim.h"
#include "model.h"
#include "resolver.h"
#include "ina239_user.h"
#include "hv_feedback.h"
#include "stm32f1xx_ll_tim.h"

void proccessor_generation_on_enter (void) {
    calc_reset_integrator();
    calc_integrator_Init();
    generation_step = 0;

    /*
     * Скважность обнуляем ДО включения выходов: в CCR лежит остаток от
     * предыдущей проверки, и без этого генератор рванул бы сразу на него.
     */
    TIM1_set_CCR_value(0);

    const Params_TestParam_TypeDef *testparams = test_results.current_test_params;
    calc_integrator_set_target_voltage(hv_feedback_volts_to_target_uV(testparams->min_voltage));

    /* Заранее вычисляем пороги для данного теста */
    resolver_fill_thresholds_with_current_testparams();

    INA239_SetMode(INA239_MODE_CONT_SHUNT_BUS);
    TIM3_Start_Count();
    calc_integrator_set_start_time();
    TIM1_Start_PWM();
    return;
}

void proccessor_generation_on_exit (void) {
    proccessor_shutdown_generation();
    INA239_SetMode(INA239_MODE_SHUTDOWN);
    commutator_deny_commutation();
    /*
     * Интегратор здесь НЕ сбрасываем, хотя напрашивается. Главный цикл
     * вызывает on_exit прежнего состояния до on_enter нового, а нового -
     * TestPassed или TestFailed - первым делом читает calc_get_test_data().
     * Сброс на этом месте обнулял бы напряжение и ток ровно перед тем, как
     * их запишут, и в протокол ложились бы нули. Сбрасывает тот, кто
     * начинает следующую генерацию, - proccessor_generation_on_enter().
     */
    return;
}

void proccessor_generation_proccess (void) {
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
        if (current_nA >= CALC_CURRENT_HIGH_nA) {
            /* Ток устройства превысил верхний порог тока теста */
            if ((voltage_uV >= voltage_lowth_uV) && (voltage_uV < voltage_highth_uV)) {
                /* Ток превысил верхний порог, но напряжение держится в диапазоне
                 * Устройство в рабочем состоянии, тестер не успел считать момент, 
                 * когда ток был в диапазоне
                 */
                proccessor_raise_event(DeviceCurrentWithinTheRange);
            } else {
                /* Ток превысил верхний порог и напряжение не в диапазоне. Поломка */
                proccessor_raise_event(DeviceCurrentOverCurrent);
            }
        } else if (voltage_uV >= voltage_highth_uV) {
            /* Напряжение превысило максимальное значение, точно поломка */
            proccessor_raise_event(DeviceVoltageOverVoltage);
        } else if (voltage_uV < voltage_lowth_uV) {
            /* Напряжение не достигло минимального значения, ждём ещё */
            proccessor_raise_event(DeviceVoltageUnderVoltage);
        } else {
            /* Напряжение в диапазоне, проверяем ток */
            if (current_nA < CALC_CURRENT_LOW_nA) {
                /* Ток устройства ещё не достиг тока теста */
                proccessor_raise_event(DeviceCurrentUnderCurrent);
            } else {
                /* Ток устройства в диапазоне тока теста */
                proccessor_raise_event(DeviceCurrentWithinTheRange);
            }
        }
    }
    return;
}

State proccessor_generation_handle_event (Event event) {
    switch (event) {
        case DeviceCurrentWithinTheRange: return TestPassed;

        case DeviceVoltageOverVoltage:
        case DeviceCurrentOverCurrent: return TestFailed;

        case DeviceCurrentUnderCurrent: 
        case DeviceVoltageUnderVoltage: 
            if (calc_is_voltage_achieved()) {
                generation_step++;
                if (!proccessor_apply_setpoint()) {
                    return TestFailed;
                }
            }
            return Generation;

        case AlertActive: return HardwareFail;

        default: return Generation;
    }
}