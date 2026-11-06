#include "states/generation.h"
#include "calc.h"
#include "display.h"
#include "ina239.h"
#include "keyboard.h"
#include "proccessor.h"
#include "screens.h"
#include "spi.h"
#include "system_stm32f1xx.h"
#include "tim.h"
#include "model.h"
#include "resolver.h"
#include "ina239.h"
#include "ina239_user.h"
#include "stm32f1xx_ll_tim.h"
#include "utils.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

/** @brief Сколько ждать выхода на уставку, мс. */
#define GENERATION_STEP_TIMEOUT_MS (1000U)

bool data_was = false;
static uint8_t num_pressed_cancel = 0;
static const uint8_t need_num_pressed_cancel = 3;
int32_t voltage_max_uV = 0;
int32_t voltage_max_buf[MAX_TESTS_FOR_ONE_GDT] = { 0 };
volatile uint8_t test_id = 0;



static bool is_data_old (void) {
    if (!data_was) {
        return false;
    }
    uint32_t last_signalize_time = spi2ctrl.get_last_signalize_time();
    uint32_t polling_period = LL_TIM_GetAutoReload(TIM3) + 1;
    uint32_t timeout_time = last_signalize_time + polling_period * 100UL * (SystemCoreClock / 1000000UL);
    uint32_t cur_CYCCNT = DWT->CYCCNT;
    if (timeout_time > last_signalize_time) {
        return (cur_CYCCNT > timeout_time);
    }
    // Время таймаута меньше времени последнего сигнала, будет переполнение
    if (cur_CYCCNT < last_signalize_time) {
        // произошло переполнение, ждём таймаута
        return (cur_CYCCNT > timeout_time);
    } else {
        return false;
    }
}

static void proccess_user_test_deny (void) {
    static Keyboard_ActiveButton prev_active = NoActive;
    Keyboard_ActiveButton cur_active = keyboard_get_active_button();
    if (cur_active != prev_active) {
        if (cur_active == CancelActive) {
            num_pressed_cancel++;
        }
        prev_active = cur_active;
    }
    return;
}



void proccessor_generation_on_enter_checkdevice (void) {
    if (!calc_integrator_Init()) {
        proccessor_shutdown_generation();
        proccessor_raise_event(ReturnToIdle);
        return;
    }
    const Params_TestParam_TypeDef *testparams = test_runtime.current_test_params;

    if (test_runtime.current_type == (uint8_t)GasDischargeTube) {
        calc_set_test_mode(CalcTestMode_SweepGDT);
        if (INA239_WriteShuntOvervoltageLimit(INA239_GetInstance(), SOVL_2mA) != INA239_OK) {
            proccessor_raise_event(ReturnToIdle);
            return;
        }
        if (INA239_WriteShuntUndervoltageLimit(INA239_GetInstance(), -SOVL_2mA) != INA239_OK) {
            proccessor_raise_event(ReturnToIdle);
            return;
        }
    } else {
        calc_set_test_mode(CalcTestMode_SweepTVS);
    }
    calc_set_test_params(testparams);

    data_was = false;
    TIM1_set_CCR_value(0);

    if (!calc_start_sweep()) {
        proccessor_shutdown_generation();
        proccessor_raise_event(ReturnToIdle);
        return;
    }

    if (INA239_SetMode(INA239_MODE_CONT_SHUNT_BUS) != INA239_OK) {
        calc_reset_sweep();
        proccessor_shutdown_generation();
        proccessor_raise_event(ReturnToIdle);
        return;
    }
    TIM3_Start_Count();
    calc_integrator_set_start_time();
    TIM1_Start_PWM();
    return;
}

void proccessor_generation_on_exit_checkdevice_default (void) {
    calc_update_metrics();
    model_remember_result((ResolveType)test_runtime.current_type);
    proccessor_shutdown_generation();
    INA239_SetMode(INA239_MODE_SHUTDOWN);
    commutator_deny_commutation();
    calc_integrator_reset();
    return;
}

void proccessor_generation_proccess_checkdevice_default (void) {
    if (have_new_data) {
        data_was = true;
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
    } else if (is_data_old()) {
        proccessor_shutdown_generation();
        proccessor_raise_event(ReturnToIdle);
    }
    proccess_user_test_deny();
    return;
}

State proccessor_generation_handle_event_checkdevice_default (Event event) {
    if (num_pressed_cancel == need_num_pressed_cancel) { 
        proccessor_force_raise_event(ReturnToIdle); 
    }

    switch (event) {
        case DeviceCurrentWithinTheRange: 
            num_pressed_cancel = 0;
            return TestPassed;

        case DeviceVoltageOverVoltage:
        case DeviceCurrentOverCurrent: 
        case DeviceSetpointExhausted:
        case DeviceVoltageSettleTimeout:
            num_pressed_cancel = 0;
            return TestFailed;

        case DeviceCurrentUnderCurrent: 
        case DeviceVoltageUnderVoltage: return Generation;

        case AlertActive: 
            num_pressed_cancel = 0;
            return HardwareFail;

        case ReturnToIdle:
            if (num_pressed_cancel == need_num_pressed_cancel) {
                num_pressed_cancel = 0;
                display_set_screen(ScreenTestDenied);
                display_allow_render();
                return Idle_Setting;
            }
            num_pressed_cancel = 0;
            return TestSequenceComplete;

        default: return Generation;
    }
}







void proccessor_generation_proccess_checkdevice_for_GDT (void) {
    if (have_new_data) {
        data_was = true;
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
        calc_get_last_raw_data(&voltage_uV, &current_nA);
        if (voltage_max_uV < voltage_uV) {
            voltage_max_uV = voltage_uV;
        }
    } else if (is_data_old()) {
        proccessor_shutdown_generation();
        proccessor_raise_event(ReturnToIdle);
    }
    proccess_user_test_deny();
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

State proccessor_generation_handle_event_checkdevice_for_GDT (Event event) {
    if (num_pressed_cancel == need_num_pressed_cancel) { 
        proccessor_force_raise_event(ReturnToIdle); 
    }

    switch (event) {
        case DeviceVoltageUnderVoltage: return Generation;

        case DeviceVoltageSettleTimeout:
        case DeviceVoltageOverVoltage:
        case DeviceSetpointExhausted: {
            if (test_id + 1 < MAX_TESTS_FOR_ONE_GDT) {
                voltage_max_buf[test_id] = voltage_max_uV;
                voltage_max_uV = 0;
                test_id++;
                num_pressed_cancel = 0;
                return Commutation;
            }
            uint8_t pos = test_runtime.failed_cnt + test_runtime.passed_cnt;
            test_results[pos].sequence_pos = pos;
            test_results[pos].type = (uint8_t)GasDischargeTube;
            test_results[pos].res.voltage_uV.mean_value = 
                get_average_from_buf(voltage_max_buf, 
                                    MAX_TESTS_FOR_ONE_GDT);
            test_results[pos].res.voltage_uV.standard_deviation = 
                get_standard_deviation_from_buf(voltage_max_buf, 
                                                MAX_TESTS_FOR_ONE_GDT, 
                                                test_results[pos].res.voltage_uV.mean_value);
            test_results[pos].res.current_nA.mean_value = 0;
            voltage_max_uV = 0;
            test_id = 0;
            num_pressed_cancel = 0;
            return TestFailed;
        }
        case AlertActive: 
            num_pressed_cancel = 0;
            return HardwareFail;

        case ReturnToIdle:
            if (num_pressed_cancel == need_num_pressed_cancel) {
                display_set_screen(ScreenTestDenied);
                display_allow_render();
                num_pressed_cancel = 0;
                return Idle_Setting;
            }
            num_pressed_cancel = 0;
            return TestSequenceComplete;

        default: return Generation;
    }
}

void proccessor_generation_on_exit_checkdevice_for_GDT (void) {
    proccessor_shutdown_generation();
    INA239_SetMode(INA239_MODE_SHUTDOWN);
    commutator_deny_commutation();
    data_was = false;
    calc_integrator_reset();
    return;
}





#ifdef TESTER_MODES
/* -------------------------------------------------------------------------- */
/* Standalone operating modes                                                 */
/* -------------------------------------------------------------------------- */

#include "window.h"

#define OMMETER_WINDOW_LENGTH          (32U)
#define MANUAL_MEASUREMENT_TIMEOUT_MS  (20U)
#define OMMETER_TIMEOUT_SECONDS        (3U)
#define OMMETER_EXCITATION_VOLTAGE_V   (5U)
#define GENERATOR_CURRENT_LIMIT_nA     (1000000L)
#define HV_FEEDBACK_DIVIDER_MILLI      (7274LL)

static uint16_t manual_target_voltage_v = 1U;
static OmmeterStatus ommeter_status = Ommeter_Invalid;
static int32_t ommeter_resistance_ohm = 0;
static Window_TypeDef ommeter_current_window = { 0 };
static int32_t ommeter_current_buf[OMMETER_WINDOW_LENGTH] = { 0 };
static int64_t ommeter_current_sum = 0;
static int64_t ommeter_current_square_sum = 0;

void proccessor_set_manual_target_voltage (uint16_t volts) {
    if (volts < 1U) {
        volts = 1U;
    }
    if (volts > HV_FEEDBACK_MAX_TEST_V) {
        volts = HV_FEEDBACK_MAX_TEST_V;
    }
    manual_target_voltage_v = volts;
    return;
}

uint16_t proccessor_get_manual_target_voltage (void) {
    return manual_target_voltage_v;
}

static void ommeter_get_sum_and_squaresum (int64_t *sum, int64_t *squaresum, Window_TypeDef *window) {
    *sum = 0;
    *squaresum = 0;
    for (uint8_t i = 0; i < window->cnt; i++) {
        int64_t cur_data = (int64_t)*((int32_t *)window->buf + i);
        *sum += cur_data;
        *squaresum += cur_data * cur_data;
    }
}

static void ommeter_set_status (OmmeterStatus status) {
    proccessor_shutdown_generation();
    (void)INA239_SetMode(INA239_MODE_SHUTDOWN);
    ommeter_status = status;
    return;
}





void proccessor_generation_on_enter_voltmeter (void) {
    proccessor_shutdown_generation();
    have_new_data = false;
    // calc_reset_integrator();
    if (INA239_WriteShuntOvervoltageLimit(INA239_GetInstance(),
                                          (int16_t)SOVL_RAW) != INA239_OK ||
        INA239_SetMode(INA239_MODE_CONT_SHUNT_BUS) != INA239_OK) {
        proccessor_raise_event(ReturnToIdle);
        return;
    }
    TIM3_Start_Count();
    return;
}

void proccessor_generation_on_exit_voltmeter (void) {
    proccessor_shutdown_generation();
    (void)INA239_SetMode(INA239_MODE_SHUTDOWN);
    commutator_deny_commutation();
    data_was = false;
    calc_reset_integrator();
    return;
}

void proccessor_generation_proccess_voltmeter (void) {
    if (have_new_data) {
        data_was = true;
        have_new_data = false;
        calc_measurement_update();
        proccessor_raise_event(OutputData);
    } else if (data_was) {
        uint32_t last_signalize_time = spi2ctrl.get_last_signalize_time();
        uint32_t polling_period = LL_TIM_GetAutoReload(TIM3) + 1;
        uint32_t timeout_time = last_signalize_time + polling_period * 100;
        if (DWT->CYCCNT > timeout_time) {
            proccessor_shutdown_generation();
            (void)INA239_SetMode(INA239_MODE_SHUTDOWN);
            proccessor_raise_event(ReturnToIdle);
        }
    }
    return;
}

State proccessor_generation_handle_event_voltmeter (Event event) {
    switch (event) {
        case OutputData: 
            display_allow_render();
            proccessor_generation_on_exit_voltmeter();
            return Generation;

        case DataOutputed: 
            display_deny_render();
            proccessor_generation_on_enter_voltmeter();
            return Generation;

        case ReturnToIdle: return Idle_Setting;
        case AlertActive: return HardwareFail;
        default: return Generation;
    }
}






void proccessor_generation_on_enter_ommeter (void) {
    // calc_reset_integrator();
    have_new_data = false;
    window_init(&ommeter_current_window, 
                sizeof(int32_t), 
                OMMETER_WINDOW_LENGTH, 
                ommeter_current_buf);
    ommeter_status = Ommeter_Invalid;
    ommeter_resistance_ohm = 0;

    TIM1_set_CCR_value(0U);
    if (!calc_integrator_Init()) {
        ommeter_set_status(Ommeter_Error);
        proccessor_raise_event(ReturnToIdle);
        return;
    }
    // calc_integrator_set_target_voltage(
    //     hv_feedback_volts_to_target_uV(OMMETER_EXCITATION_VOLTAGE_V));

    // if (!delay_register_task(&delay_step, GENERATION_STEP_TIMEOUT_MS, Millis)) {
    //     ommeter_set_status(Ommeter_Error);
    //     proccessor_raise_event(ReturnToIdle);
    //     return;
    // }

    if (INA239_WriteShuntOvervoltageLimit(INA239_GetInstance(),
                                          (int16_t)SOVL_RAW) != INA239_OK ||
        INA239_SetMode(INA239_MODE_CONT_SHUNT_BUS) != INA239_OK) {
        ommeter_set_status(Ommeter_Error);
        proccessor_raise_event(ReturnToIdle);
        return;
    }

    TIM3_Start_Count();
    calc_integrator_set_start_time();
    TIM1_Start_PWM();
    return;
}

void proccessor_generation_on_exit_ommeter (void) {
    proccessor_shutdown_generation();
    (void)INA239_SetMode(INA239_MODE_SHUTDOWN);
    commutator_deny_commutation();
    have_new_data = false;
    window_reset(&ommeter_current_window);
    calc_reset_integrator();
    return;
}

void proccessor_generation_proccess_ommeter (void) {
    if (have_new_data) {
        data_was = true;
        have_new_data = false;
        calc_routine();

        /* Не измеряем сопротивление на переходном процессе регулятора. */
        if (!calc_is_voltage_achieved()) {
            ommeter_set_status(Ommeter_Invalid);
            return;
        }

        int32_t voltage_uV = 0;
        int32_t current_nA = 0;
        calc_get_test_data(&voltage_uV, &current_nA);
        window_push(&ommeter_current_window, &current_nA);
        ommeter_get_sum_and_squaresum(&ommeter_current_sum, 
                                      &ommeter_current_square_sum, 
                                      &ommeter_current_window);
        int64_t n = (int64_t)OMMETER_WINDOW_LENGTH;
        int64_t variance_numerator = n * ommeter_current_square_sum -
                                    ommeter_current_sum * ommeter_current_sum;
        if (variance_numerator < 0) {
            variance_numerator = 0;
        }
        if ((100LL * variance_numerator) <=
            (ommeter_current_sum * ommeter_current_sum)) {
            ommeter_set_status(Ommeter_Invalid);
            return;
        }

        int32_t mean_current_nA = ommeter_current_sum / 
                ((ommeter_current_window.cnt != 0) ? ommeter_current_window.cnt : 1);
        if (mean_current_nA <= 0 || voltage_uV <= 0) {
            ommeter_set_status(Ommeter_Invalid);
            return;
        }

        /*
        * voltage_uV is measured before the high-voltage divider.  R[Ohm] =
        * U_feedback[uV] * divider * 1000 / I[nA]. Divider is represented as
        * 7274/1000 to avoid floating point on Cortex-M3.
        */
        int64_t resistance = ((int64_t)voltage_uV * HV_FEEDBACK_DIVIDER_MILLI) /
                            (int64_t)mean_current_nA;
        if (resistance <= 0 || resistance > INT32_MAX) {
            ommeter_set_status(Ommeter_Invalid);
            return;
        }

        ommeter_resistance_ohm = (int32_t)resistance;
        ommeter_set_status(Ommeter_Valid);
        proccessor_raise_event(OutputData);
        return;
    } else if (data_was) {
        uint32_t last_signalize_time = spi2ctrl.get_last_signalize_time();
        uint32_t polling_period = LL_TIM_GetAutoReload(TIM3) + 1;
        uint32_t timeout_time = last_signalize_time + polling_period * 100;
        if (DWT->CYCCNT > timeout_time) {
            proccessor_shutdown_generation();
            proccessor_raise_event(ReturnToIdle);
        }
        return;
    }
}

State proccessor_generation_handle_event_ommeter (Event event) {
    switch (event) {
        case OutputData: 
            display_allow_render();
            proccessor_generation_on_exit_ommeter();
            return Generation;

        case DataOutputed: 
            display_deny_render();
            proccessor_generation_on_enter_ommeter();
            return Generation;

        case ReturnToIdle: return Idle_Setting;
        case AlertActive: return HardwareFail;
        default: return Generation;
    }
}

OmmeterStatus proccessor_ommeter_get_status (void) {
    return ommeter_status;
}

int32_t proccessor_ommeter_get_resistance_ohm (void) {
    return ommeter_resistance_ohm;
}







void proccessor_generation_on_enter_generator (void) {
    // calc_reset_integrator();
    have_new_data = false;
    generation_step = 0U;
    TIM1_set_CCR_value(0U);

    if (!calc_integrator_Init()) {
        proccessor_raise_event(ReturnToIdle);
        return;
    }
    // calc_integrator_set_target_voltage(
    //     hv_feedback_volts_to_target_uV(manual_target_voltage_v));

    // if (!delay_register_task(&delay_step, GENERATION_STEP_TIMEOUT_MS, Millis)) {
    //     proccessor_raise_event(ReturnToIdle);
    //     return;
    // }

    /* 1 mA -> 22 mV across the 22 Ohm shunt -> 4400 raw LSB (5 uV/LSB). */
    const int16_t current_limit_raw = (int16_t)(
        hv_shunt_nA_to_uV(GENERATOR_CURRENT_LIMIT_nA) /
        (int32_t)INA239_VSHUNT_LSB_RANGE0_uV);
    if (INA239_WriteShuntOvervoltageLimit(INA239_GetInstance(), current_limit_raw) != INA239_OK ||
        INA239_SetMode(INA239_MODE_CONT_SHUNT_BUS) != INA239_OK) {
        proccessor_shutdown_generation();
        (void)INA239_SetMode(INA239_MODE_SHUTDOWN);
        proccessor_raise_event(ReturnToIdle);
        return;
    }

    TIM3_Start_Count();
    calc_integrator_set_start_time();
    TIM1_Start_PWM();
    return;
}

void proccessor_generation_on_exit_generator (void) {
    proccessor_shutdown_generation();
    (void)INA239_SetMode(INA239_MODE_SHUTDOWN);
    commutator_deny_commutation();
    /* Keep the 1 mA threshold until HardwareFail has read the latched alert. */
    calc_reset_integrator();
    return;
}

void proccessor_generation_proccess_generator (void) {
    if (!have_new_data) {
        uint32_t last_signalize_time = spi2ctrl.get_last_signalize_time();
        uint32_t polling_period = LL_TIM_GetAutoReload(TIM3) + 1;
        uint32_t timeout_time = last_signalize_time + polling_period * 100;
        if (DWT->CYCCNT > timeout_time) {
            proccessor_shutdown_generation();
            (void)INA239_SetMode(INA239_MODE_SHUTDOWN);
            proccessor_raise_event(ReturnToIdle);
        }
        return;
    }
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
    if (new_CCR1_val != LL_TIM_OC_GetCompareCH1(TIM1)) {
        TIM1_set_CCR_value(new_CCR1_val);
    }

    int32_t voltage_uV = 0;
    int32_t current_nA = 0;
    calc_get_test_data(&voltage_uV, &current_nA);
    (void)voltage_uV;
    if (current_nA >= GENERATOR_CURRENT_LIMIT_nA) {
        proccessor_shutdown_generation();
        proccessor_raise_event(ReturnToIdle);
    }
    return;
}

State proccessor_generation_handle_event_generator (Event event) {
    switch (event) {
        case ReturnToIdle: return Idle_Setting;
        case AlertActive: return HardwareFail;
        default: return Generation;
    }
}
#endif