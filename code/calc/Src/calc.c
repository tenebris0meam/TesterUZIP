#include "calc.h"
#include "critical_section.h"
#include "delay.h"
#include "hv_feedback.h"
#include "ina239.h"
#include "ina239_user.h"
#include "proccessor.h"
#include "system_stm32f1xx.h"
#include "utils.h"
#include "tim.h"
#include "window.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define CALC_WINDOW_LENGTH (4)
#define CALC_WINDOW_LENGTH_POWER2 (2)

#define CALC_DERIVATIVE_VOLTAGE_MAX (68807339LL) // 500 V/s с учётом делителя напряжения
#define CALC_SWEEP_TIMEOUT_MS (3000U)

#define CALC_uV_TO_PPM(error_uV) (error_uV)
#define CALC_nA_TO_PPM(error_nA) ((error_nA) * 2)

#define CALC_CC_ENTER_nA (100000L) // 0.1 мА
#define CALC_CC_TIMEOUT_MS (3000U) // 3 с


typedef enum {
    VOLTAGE,
    CURRENT
} RawValType;

/** @brief Состояние интегратора регулятора напряжения. */
typedef struct {
    uint32_t target_voltage_uV;  /**< Уставка после делителя, мкВ */
    uint32_t ramp_voltage_uV;  /**< Текущее значение рампы уставки, мкВ */
    uint32_t start_time_integral;  /**< CYCCNT предыдущего шага интегратора */
    int32_t integral; /**< Накопленное значение; скважность в миллионных долях (ppm) */
} IntegratorTypeDef;

volatile bool have_new_data = false;

static IntegratorTypeDef integrator = { 0 };
static CalcMetrics_TypeDef metrics = { 0 };

static bool is_voltage_achieved = false;
static bool is_current_limitting = false;
static int32_t voltage_lowth = 0;
static int32_t voltage_highth = 0;

/* --- sweep / CC state --- */
static CalcTestMode test_mode = CalcTestMode_SweepTVS;
static const Params_TestParam_TypeDef *test_params = NULL;
static int32_t test_voltage_min_uV = 0;
static int32_t test_voltage_max_uV = 0;
static uint8_t generation_step = 0;
static uint8_t delay_step = 0;
static bool delay_registered = false;

static bool is_cc_mode = false;
static int32_t cc_integral_ppm = 0;
static uint32_t cc_last_time = 0;
static uint32_t cc_enter_time = 0;



/**
 * @brief Снимок кадра ответа INA239.
 * @details Читать answer_vbus_and_current напрямую нельзя: DMA переписывает
 *          его каждые 2*T_conv по прерыванию TIM3, и между чтением напряжения
 *          и чтением тока может прийти новый кадр. Тогда регулятор получит
 *          напряжение из одного измерения, а ток из следующего.
 */
static void calc_snapshot_frame (uint8_t *frame) {
    enter_critsec();
    for (uint32_t i = 0; i < INA239_FRAME_SIZE; i++) {
        frame[i] = answer_vbus_and_current[i];
    }
    exit_critsec();
    return;
}

static int16_t calc_get_raw_val (const uint8_t *frame, RawValType type) {
    switch (type) {
        case VOLTAGE: return (int16_t)(((uint16_t)frame[1] << 8) | frame[2]);
        case CURRENT: return (int16_t)(((uint16_t)frame[4] << 8) | frame[5]);
        default: return 0;
    }
}

static int32_t calc_get_average_from_window (Window_TypeDef *window) {
    int64_t acc = 0;
    for (uint32_t i = 0; i < window->num_elements; i++) {
        acc += (int64_t)*((int32_t *)window->buf + i);
    }
    // arm-none-eabi-gcc гарантирует арифметический сдвиг
    acc >>= CALC_WINDOW_LENGTH_POWER2;
    return (int32_t)acc;
}

static int32_t calc_get_standard_deviation_from_window (Window_TypeDef *window, int32_t mean_val) {
    int64_t acc = 0;
    for (uint32_t i = 0; i < window->num_elements; i++) {
        int64_t cur_delta = (int64_t)*((int32_t *)window->buf + i) - (int64_t)mean_val;
        acc += cur_delta * cur_delta;
    }
    acc /= window->num_elements - 1;
    return (int32_t)uint64_sqrt(acc);
}

/* --- CC integrator --- */
static void calc_enter_cc_mode (void) {
    is_cc_mode = true;
    cc_integral_ppm = integrator.integral;
    cc_last_time = DWT->CYCCNT;
    cc_enter_time = DWT->CYCCNT;
    /* Замораживаем уставку: рампа не должна продолжать уходить вверх */
    integrator.target_voltage_uV = integrator.ramp_voltage_uV;
    voltage_lowth = (100 - CALC_TARGET_TOLERANCE_PERCENT) * (integrator.target_voltage_uV / 100);
    voltage_highth = (100 + CALC_TARGET_TOLERANCE_PERCENT) * (integrator.target_voltage_uV / 100);
    return;
}

static void calc_cc_step (int32_t measured_nA) {
    uint32_t now = DWT->CYCCNT;
    uint32_t dt = now - cc_last_time;
    cc_last_time = now;

    int32_t error_nA = CALC_CURRENT_NOMINAL_nA - measured_nA;
    int64_t delta = CALC_nA_TO_PPM((int64_t)error_nA) * (int64_t)dt / (int64_t)SystemCoreClock;

    int64_t sum = (int64_t)cc_integral_ppm + delta;
    if (sum < 0) {
        sum = 0;
    }
    if (sum > CALC_DUTY_MAX_PPM) {
        sum = CALC_DUTY_MAX_PPM;
    }
    cc_integral_ppm = (int32_t)sum;
    return;
}

static bool calc_apply_setpoint_internal (void) {
    if (test_params == NULL) {
        return false;
    }
    if (generation_step > GENERATION_MAX_STEPS) {
        return false;
    }

    uint32_t max_volt = hv_feedback_volts_to_target_uV(test_params->max_voltage);
    uint32_t min_volt = hv_feedback_volts_to_target_uV(test_params->min_voltage);
    uint32_t span = max_volt - min_volt;
    if ((span == 0U) && (generation_step > 0U)) {
        return false;
    }

    uint32_t volts = min_volt + (span * generation_step) / GENERATION_MAX_STEPS;
    if (volts > HV_FEEDBACK_BUS_LIMIT_uV) { 
        volts = HV_FEEDBACK_BUS_LIMIT_uV;
    }
    calc_integrator_set_target_voltage(volts);
    return true;
}

/* --- sweep step (shared TVS/VAR + GDT) --- */
static void calc_sweep_step (Event continue_event) {
    if (!delay_is_not_yet_expired(delay_step)) {
        proccessor_raise_event(DeviceVoltageSettleTimeout);
        return;
    }
    if (!is_voltage_achieved) {
        proccessor_raise_event(continue_event);
        return;
    }
    
    /* 
     * Если мы уже на последнем шаге (max_voltage), не исчерпываем счётчик.
     * Мы должны удерживать напряжение и ждать, пока ток вырастет до порога CC
     * или пока не сработает таймаут ожидания стабилизации.
     */
    if ((metrics.voltage_uV >= test_voltage_min_uV) && (metrics.voltage_uV < test_voltage_max_uV)) {
        if (generation_step >= GENERATION_MAX_STEPS) {
            proccessor_raise_event(continue_event);
            return;
        }
        generation_step++;
    }

    if (!calc_apply_setpoint_internal()) {
        proccessor_raise_event(DeviceSetpointExhausted);
        return;
    }
    delay_update_task(delay_step);
    proccessor_raise_event(continue_event);
    return;
}

/* --- TVS/VAR classification --- */
static void calc_tvs_var_classify (int32_t v, int32_t i) {
    /* приоритет 1: overcurrent */
    if (i >= CALC_CURRENT_HIGH_nA) {
        if ((v >= test_voltage_min_uV) && (v < test_voltage_max_uV)) {
            proccessor_raise_event(DeviceCurrentWithinTheRange);
        } else {
            proccessor_raise_event(DeviceCurrentOverCurrent);
        }
        return;
    }
    /* приоритет 2: overvoltage */
    if (v >= test_voltage_max_uV) {
        proccessor_raise_event(DeviceVoltageOverVoltage);
        return;
    }
    /* приоритет 3: undervoltage — sweep */
    if (v < test_voltage_min_uV) {
        calc_sweep_step(DeviceVoltageUnderVoltage);
        return;
    }
    /* напряжение в диапазоне */
    if (is_cc_mode) {
        if (i >= CALC_CURRENT_NOMINAL_nA) {
            proccessor_raise_event(DeviceCurrentWithinTheRange);
        } else {
            uint32_t elapsed = (DWT->CYCCNT - cc_enter_time) / (SystemCoreClock / 1000U);
            if (elapsed > CALC_CC_TIMEOUT_MS) {
                proccessor_raise_event(DeviceVoltageSettleTimeout);
            } else {
                proccessor_raise_event(DeviceCurrentUnderCurrent);
            }
        }
        return;
    }
    /* вход в CC: ток подошёл к 1 мА, регулятор напряжения устоялся */
    if (i >= CALC_CC_ENTER_nA) {
        calc_enter_cc_mode();
        proccessor_raise_event(DeviceCurrentUnderCurrent);
        return;
    }
    /* обычный sweep */
    if (i < CALC_CURRENT_NOMINAL_nA) {
        calc_sweep_step(DeviceCurrentUnderCurrent);
    } else {
        proccessor_raise_event(DeviceCurrentWithinTheRange);
    }
}

/* --- GDT classification --- */
static void calc_gdt_classify (int32_t v) {
    if (v >= test_voltage_max_uV) {
        proccessor_raise_event(DeviceVoltageOverVoltage);
    } else {
        calc_sweep_step(DeviceVoltageUnderVoltage);
    }
}

void calc_set_test_mode (CalcTestMode mode) {
    test_mode = mode;
    return;
}

void calc_set_test_params (const Params_TestParam_TypeDef *tp) {
    test_params = tp;
    if (tp) {
        test_voltage_min_uV = (int32_t)hv_feedback_volts_to_target_uV(tp->min_voltage);
        test_voltage_max_uV = (int32_t)hv_feedback_volts_to_target_uV(tp->max_voltage);
    } else {
        test_voltage_min_uV = 0;
        test_voltage_max_uV = 0;
    }
}

bool calc_start_sweep (void) {
    calc_reset_sweep();
    if (test_params == NULL) {
        return false;
    }
    if (!delay_register_task(&delay_step, CALC_SWEEP_TIMEOUT_MS, Millis)) {
        return false;
    }
    delay_registered = true;
    generation_step = 0;
    if (!calc_apply_setpoint_internal()) {
        delay_unregister_task(delay_step);
        delay_registered = false;
        return false;
    }
    return true;
}

void calc_reset_sweep (void) {
    if (delay_registered) {
        delay_unregister_task(delay_step);
        delay_registered = false;
    }
    generation_step = 0;
    is_cc_mode = false;
    cc_integral_ppm = 0;
    cc_last_time = 0;
    cc_enter_time = 0;
    return;
}

bool calc_integrator_Init (void) {
    INA239_Handle_t *hina = INA239_GetInstance();

    uint16_t cur_adc_config;
    if (INA239_ReadAdcConfig(hina, &cur_adc_config) != INA239_OK) {
        return false;
    }
    INA239_ConvTime_t conv_time_vbus = 
        (INA239_ConvTime_t)((cur_adc_config & INA239_ADCCFG_VBUSCT_Msk) >> INA239_ADCCFG_VBUSCT_Pos);
    INA239_ConvTime_t conv_time_shunt = 
        (INA239_ConvTime_t)((cur_adc_config & INA239_ADCCFG_VSHCT_Msk) >> INA239_ADCCFG_VSHCT_Pos);
    INA239_Avg_t avg_vbus_shunt = 
        (INA239_Avg_t)((cur_adc_config & INA239_ADCCFG_AVG_Msk) >> INA239_ADCCFG_AVG_Pos);

    uint32_t delta_time_us = (get_val_from_conv_time(conv_time_vbus) + 
                              get_val_from_conv_time(conv_time_shunt)) * 
                              get_val_from_avg(avg_vbus_shunt);

    TIM3_Change_Update_Time(delta_time_us);

    /*
     * Уставку здесь НЕ трогаем: её ставит процессор под конкретный прибор.
     */
    return true;
}

void calc_integrator_set_target_voltage (uint32_t target_voltage_after_div_uV) {
    if (integrator.target_voltage_uV != target_voltage_after_div_uV) {
        integrator.target_voltage_uV = target_voltage_after_div_uV;
        voltage_lowth = (100 - CALC_TARGET_TOLERANCE_PERCENT) * (integrator.target_voltage_uV / 100);
        voltage_highth = (100 + CALC_TARGET_TOLERANCE_PERCENT) * (integrator.target_voltage_uV / 100);
        is_voltage_achieved = false;
    }
    return;
}

void calc_integrator_set_start_time (void) {
    integrator.start_time_integral = DWT->CYCCNT;
    return;
}

uint32_t calc_integrator_get_target_voltage (void) {
    return integrator.target_voltage_uV;
}

int32_t calc_integrator_get_integral_value (void) {
    return is_cc_mode ? cc_integral_ppm : integrator.integral;
}

void calc_integrator_reset (void) {
    memset(&integrator, 0, sizeof(IntegratorTypeDef));
    is_voltage_achieved = false;
    is_current_limitting = false;
    voltage_lowth = 0;
    voltage_highth = 0;
    memset(&metrics, 0, sizeof(CalcMetrics_TypeDef));
    calc_reset_sweep();
    return;
}

/**
 * @brief Один шаг интегратора.
 * @details Скважность хранится в миллионных долях, а не в вольтах: именно её
 *          процессор кладёт в CCR таймера. Ограничение снизу нулём не даёт
 *          регулятору уйти в отрицательную область, сверху - выйти за
 *          безопасную скважность.
 */
static void calc_voltage_integrator_step  (int32_t measured_uV) {
    if (!is_voltage_achieved && ((measured_uV >= voltage_lowth) && (measured_uV < voltage_highth))) {
        is_voltage_achieved = true;
    } else if (is_voltage_achieved && ((measured_uV < voltage_lowth) || (measured_uV >= voltage_highth))) {
        is_voltage_achieved = false;
    }

    uint32_t cur_CYCCNT = DWT->CYCCNT;
    uint32_t delta_tick = cur_CYCCNT - integrator.start_time_integral; 
    
    int64_t step = CALC_DERIVATIVE_VOLTAGE_MAX * (int64_t)delta_tick / (int64_t)SystemCoreClock;
    int32_t ramp_diff = integrator.target_voltage_uV - integrator.ramp_voltage_uV;
    if (ramp_diff > step) {
        integrator.ramp_voltage_uV += step;
    } else if (ramp_diff < -step) {
        integrator.ramp_voltage_uV -= step;
    } else {
        integrator.ramp_voltage_uV = integrator.target_voltage_uV;
    }
                          
    int32_t error_uV = (int32_t)integrator.ramp_voltage_uV - measured_uV;
    int32_t delta_ppm = (int32_t)((int64_t)error_uV * (int64_t)delta_tick / (int64_t)SystemCoreClock);
    int32_t sum_ppm = integrator.integral + delta_ppm;

    if (sum_ppm < 0) {
        sum_ppm = 0;
    }
    if (sum_ppm > CALC_DUTY_MAX_PPM) {
        sum_ppm = CALC_DUTY_MAX_PPM;
    }
    integrator.integral = sum_ppm;
    calc_integrator_set_start_time();
    return;
}

void calc_measurement_update (void) {
    uint8_t frame[INA239_FRAME_SIZE];
    calc_snapshot_frame(frame);

    int16_t voltage_raw = calc_get_raw_val(frame, VOLTAGE);
    int16_t current_raw = calc_get_raw_val(frame, CURRENT);

    int32_t shunt_dropvoltage = (int32_t)current_raw * (int32_t)INA239_VSHUNT_LSB_RANGE0_uV;
    metrics.voltage_uV = (int32_t)voltage_raw * (int32_t)INA239_VBUS_LSB_uV - shunt_dropvoltage;
    metrics.current_nA = hv_shunt_uV_to_nA(shunt_dropvoltage);
    return;
}

void calc_step (void) {
    if ((test_mode == CalcTestMode_SweepGDT) || 
        ((test_mode == CalcTestMode_SweepTVS) && !is_cc_mode)) {
        calc_voltage_integrator_step(metrics.voltage_uV);
    } else if ((test_mode == CalcTestMode_SweepTVS) && is_cc_mode) {
        calc_cc_step(metrics.current_nA);
    }
}

void calc_control (void) {
    if (test_mode == CalcTestMode_SweepGDT) {
        calc_gdt_classify(metrics.voltage_uV);
    } else {
        calc_tvs_var_classify(metrics.voltage_uV, metrics.current_nA);
    }
    return;
}

const CalcMetrics_TypeDef *calc_get_current_metrics (void) {
    return (const CalcMetrics_TypeDef *)&metrics;
}