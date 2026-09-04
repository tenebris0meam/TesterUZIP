#include "calc.h"
#include "hv_feedback.h"
#include "ina239.h"
#include "ina239_user.h"
#include "utils.h"
#include "stm32f103xb.h"   /* __disable_irq / __get_PRIMASK */
#include "tim.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>



volatile bool have_new_data = false;

static IntegratorTypeDef integrator = { 0 };

/** @brief Актуальные данные о напряжении и токе теста */
static int32_t voltage_test = 0;
static int32_t current_test = 0;



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
    calc_integrator_set_delta_time(delta_time_us);

    /*
     * Уставку здесь НЕ трогаем: её ставит процессор под конкретный прибор.
     * Ноль означает "не разгонять" - безопасное состояние по умолчанию.
     */
    return true;
}

void calc_integrator_set_delta_time (uint32_t delta_time_us) {
    integrator.delta_time = (float)delta_time_us * 0.000001f;
    return;
}

void calc_integrator_set_target_voltage (uint32_t target_voltage_after_div_uV) {
    integrator.target_voltage_uV = target_voltage_after_div_uV;
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
    current_test = 0;
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
    int32_t error_uV = (int32_t)integrator.target_voltage_uV - measured_uV;
    int32_t delta_ppm = (int32_t)((float)error_uV * integrator.delta_time);
    int32_t sum_ppm = integrator.integral + delta_ppm;

    if (sum_ppm < 0) {
        sum_ppm = 0;
    }
    if (sum_ppm > CALC_DUTY_MAX_PPM) {
        sum_ppm = CALC_DUTY_MAX_PPM;
    }
    integrator.integral = sum_ppm;
    return;
}

void calc_routine (void) {
    uint8_t frame[INA239_FRAME_SIZE];
    calc_snapshot_frame(frame);

    int16_t voltage_raw = calc_get_raw_val(frame, VOLTAGE);
    int16_t current_raw = calc_get_raw_val(frame, CURRENT);

    int32_t shunt_dropvoltage = (int32_t)current_raw * (int32_t)INA239_VSHUNT_LSB_RANGE0_uV;
    voltage_test = (int32_t)voltage_raw * (int32_t)INA239_VBUS_LSB_uV;
    current_test = hv_shunt_uV_to_nA(shunt_dropvoltage);

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
