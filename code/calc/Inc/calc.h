#ifndef CALC_H
#define CALC_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @file calc.h
 * @brief Регулятор напряжения и вердикт по току испытуемого прибора.
 *
 * @details Модуль разделён на два слоя, которые ничего не знают друг о друге:
 *
 *          1. РЕГУЛЯТОР - интегратор, который по измеренному напряжению и
 *             уставке выдаёт скважность ШИМ. Про тесты не знает ничего.
 *          2. ВЕРДИКТ - разбор тока испытуемого прибора. Выносится только
 *             после того, как регулятор ВЫШЕЛ НА УСТАВКУ: пока напряжение
 *             ещё растёт, ток ничего не говорит о приборе.
 *
 *          Логику самого теста - какую уставку ставить, что делать с
 *          вердиктом - держит процессор (code/proccessor).
 */

extern volatile bool have_new_data;

/** @brief Номинальный ток испытания, нА. */
#define CALC_NOMINAL_CURRENT_nA (1000000L)

#define CALC_CURRENT_LOW_nA 900000L
#define CALC_CURRENT_HIGH_nA 1100000L

/**
 * @brief Допуск, при котором считаем, что регулятор вышел на уставку, мкВ.
 * @details 1 % от максимума шины INA239. Меньше делать нет смысла: младший
 *          разряд VBUS - 3.125 мВ.
 */
#define CALC_TARGET_TOLERANCE_PERCENT 5UL

/** @brief Верхняя граница скважности, миллионные доли. */
#define CALC_DUTY_MAX_PPM (970000L)

typedef enum {
    VOLTAGE,
    CURRENT
} RawValType;

typedef struct {
    uint32_t target_voltage_uV;
    uint32_t start_time_integral;
    int32_t integral;
} IntegratorTypeDef;

bool calc_integrator_Init (void);
void calc_integrator_set_target_voltage (uint32_t target_voltage_after_div_uV);
void calc_integrator_set_start_time (void);
uint32_t calc_integrator_get_target_voltage (void);
int32_t calc_integrator_get_integral_value (void);
void calc_reset_integrator (void);

/**
 * @brief Один шаг: снять кадр измерений, подвинуть регулятор, вынести вердикт.
 * @details Вердикт поднимается процессору событием и только после выхода на
 *          уставку.
 */
void calc_routine (void);

/** @brief Данные последнего вынесенного вердикта. */
void calc_get_test_data (int32_t *voltage, int32_t *current);
bool calc_is_voltage_achieved (void);

#endif /* CALC_H */
