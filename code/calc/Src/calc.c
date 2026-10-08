#include "calc.h"
#include "hv_feedback.h"
#include "ina239.h"
#include "ina239_user.h"
#include "system_stm32f1xx.h"
#include "utils.h"
#include "stm32f103xb.h"   /* __disable_irq / __get_PRIMASK */
#include "tim.h"
#include "window.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define CALC_WINDOW_LENGTH 32

#define CALC_DERIVATIVE_VOLTAGE_MAX (200000L)



volatile bool have_new_data = false;

static IntegratorTypeDef integrator = { 0 };
static Window_TypeDef voltage_window = { 0 };
static Window_TypeDef current_window = { 0 };
static int32_t voltage_buf[CALC_WINDOW_LENGTH] = { 0 };
static int32_t current_buf[CALC_WINDOW_LENGTH] = { 0 };

/** @brief Актуальные данные о напряжении и токе теста */
static int32_t voltage_test = 0;
static int32_t previous_error_uV = 0;
static int32_t current_test = 0;

static bool is_voltage_achieved = false;
static int32_t voltage_lowth = 0;
static int32_t voltage_highth = 0;



/**
 * @brief Снимок кадра ответа INA239.
 * @details Читать answer_vbus_and_current напрямую нельзя: DMA переписывает
 *          его каждые 100 мкс по прерыванию TIM3, и между чтением напряжения
 *          и чтением тока может прийти новый кадр. Тогда регулятор получит
 *          напряжение из одного измерения, а ток из следующего.
 */
static void calc_snapshot_frame (uint8_t *frame) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    for (uint32_t i = 0; i < INA239_FRAME_SIZE; i++) {
        frame[i] = answer_vbus_and_current[i];
    }
    __set_PRIMASK(primask);
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
    for (uint32_t i = 0; i < window->cnt; i++) {
        acc += (int64_t)*(int32_t *)(window->buf + i * window->element_size);
    }
    acc /= (int64_t)window->cnt;
    return (int32_t)acc;
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

    window_init(&voltage_window, sizeof(int32_t), CALC_WINDOW_LENGTH, voltage_buf);
    window_init(&current_window, sizeof(int32_t), CALC_WINDOW_LENGTH, current_buf);

    /*
     * Уставку здесь НЕ трогаем: её ставит процессор под конкретный прибор.
     * Ноль означает "не разгонять" - безопасное состояние по умолчанию.
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
    return integrator.integral;
}

void calc_reset_integrator (void) {
    memset(&integrator, 0, sizeof(IntegratorTypeDef));
    voltage_test = 0;
    previous_error_uV = 0;
    current_test = 0;
    is_voltage_achieved = false;
    voltage_lowth = 0;
    voltage_highth = 0;
    window_reset(&voltage_window);
    window_reset(&current_window);
    return;
}

/**
 * @brief Один шаг интегратора.
 * @details Скважность хранится в миллионных долях, а не в вольтах: именно её
 *          процессор кладёт в CCR таймера. Ограничение снизу нулём не даёт
 *          регулятору уйти в отрицательную область, сверху - выйти за
 *          безопасную скважность.
 */
static void calc_integrator_step (int32_t measured_uV) {
    if (!is_voltage_achieved && (measured_uV >= voltage_lowth) && (measured_uV < voltage_highth)) {
        is_voltage_achieved = true;
    }

    uint32_t cur_CYCCNT = DWT->CYCCNT;
    uint64_t delta_tick = (cur_CYCCNT > integrator.start_time_integral) ? 
                          (cur_CYCCNT - integrator.start_time_integral) : 
                          (UINT32_MAX - integrator.start_time_integral + cur_CYCCNT); 
    int32_t error_uV = (int32_t)integrator.target_voltage_uV - measured_uV;
    int32_t cur_error_derivative = (int64_t)(error_uV - previous_error_uV) * (int64_t)SystemCoreClock / (int64_t)delta_tick;
    if ((cur_error_derivative > CALC_DERIVATIVE_VOLTAGE_MAX) || 
        (cur_error_derivative < -CALC_DERIVATIVE_VOLTAGE_MAX)) {
        error_uV = previous_error_uV + 
            (int32_t)((int64_t)CALC_DERIVATIVE_VOLTAGE_MAX * (int64_t)delta_tick / (int64_t)SystemCoreClock);
    }
    previous_error_uV = error_uV;
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

void calc_routine (void) {
    uint8_t frame[INA239_FRAME_SIZE];
    calc_snapshot_frame(frame);

    int16_t voltage_raw = calc_get_raw_val(frame, VOLTAGE);
    int16_t current_raw = calc_get_raw_val(frame, CURRENT);

    int32_t shunt_dropvoltage = (int32_t)current_raw * (int32_t)INA239_VSHUNT_LSB_RANGE0_uV;
    int32_t u_voltage = (int32_t)voltage_raw * (int32_t)INA239_VBUS_LSB_uV - shunt_dropvoltage;
    int32_t n_amperage = hv_shunt_uV_to_nA(shunt_dropvoltage);

    window_push(&voltage_window, &u_voltage);
    window_push(&current_window, &n_amperage);

    voltage_test = calc_get_average_from_window(&voltage_window);
    current_test = calc_get_average_from_window(&current_window);

    calc_integrator_step(voltage_test);
    return;
}

void calc_get_test_data (int32_t *voltage, int32_t *current) {
    if (voltage) {
        *voltage = voltage_test;
    }
    if (current) {
        *current = current_test;
    }
    return;
}

bool calc_is_voltage_achieved (void) {
    bool voltage_achieved = is_voltage_achieved;
    is_voltage_achieved = false;
    return voltage_achieved;
}