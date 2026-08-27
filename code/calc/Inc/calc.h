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

/** @brief Допуск на ток испытания, процентов от номинала. */
#define CALC_CURRENT_TOLERANCE_PERCENT (20L)

#define CALC_CURRENT_LOW_nA \
    (CALC_NOMINAL_CURRENT_nA - \
     (CALC_NOMINAL_CURRENT_nA * CALC_CURRENT_TOLERANCE_PERCENT) / 100L)

#define CALC_CURRENT_HIGH_nA \
    (CALC_NOMINAL_CURRENT_nA + \
     (CALC_NOMINAL_CURRENT_nA * CALC_CURRENT_TOLERANCE_PERCENT) / 100L)

/**
 * @brief Допуск, при котором считаем, что регулятор вышел на уставку, мкВ.
 * @details 1 % от максимума шины INA239. Меньше делать нет смысла: младший
 *          разряд VBUS - 3.125 мВ.
 */
#define CALC_TARGET_TOLERANCE_uV (500000UL)

/**
 * @brief Сколько подряд измерений должны попасть в допуск.
 * @details При периоде опроса 100 мкс это 5 мс устойчивого удержания -
 *          защита от того, чтобы вердикт вынесли на случайном выбросе.
 */
#define CALC_SETTLE_SAMPLES (50U)

/** @brief Верхняя граница скважности, миллионные доли. */
#define CALC_DUTY_MAX_PPM (970000L)

typedef enum {
    VOLTAGE,
    CURRENT
} RawValType;

/** @brief Что регулятор думает про ток испытуемого прибора. */
typedef enum {
    CalcVerdict_NotSettled,   /**< на уставку ещё не вышли, судить рано  */
    CalcVerdict_UnderCurrent, /**< ток ниже допуска                      */
    CalcVerdict_WithinRange,  /**< ток в допуске                         */
    CalcVerdict_OverCurrent   /**< ток выше допуска или зашкал измерения */
} CalcVerdict;

typedef struct {
    uint32_t target_voltage_uV;
    float delta_time;
    int32_t integral;
} IntegratorTypeDef;

/* --------------------------- слой регулятора --------------------------- */

bool calc_integrator_Init (void);
void calc_integrator_set_delta_time (uint32_t delta_time_us);
void calc_integrator_set_target_voltage (uint32_t target_voltage_after_div_uV);
uint32_t calc_integrator_get_target_voltage (void);
int32_t calc_integrator_get_integral_value (void);
void calc_reset_integrator (void);

/** @brief Вышел ли регулятор на уставку и удерживает ли её. */
bool calc_integrator_is_settled (void);

/* ----------------------------- слой вердикта ---------------------------- */

/**
 * @brief Один шаг: снять кадр измерений, подвинуть регулятор, вынести вердикт.
 * @details Вердикт поднимается процессору событием и только после выхода на
 *          уставку.
 */
void calc_routine (void);

/** @brief Последний вердикт. */
CalcVerdict calc_get_verdict (void);

/** @brief Данные последнего вынесенного вердикта. */
void calc_get_test_data (int32_t *voltage, int32_t *current);

#endif /* CALC_H */
