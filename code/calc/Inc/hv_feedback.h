#ifndef HV_FEEDBACK_H_
#define HV_FEEDBACK_H_

#include <stdint.h>

/**
 * @file hv_feedback.h
 * @brief Связь между напряжением на выходе генератора, током после устройства и тем, что видит INA239.
 *
 * @details ЭТО ЕДИНСТВЕННОЕ МЕСТО, где живёт эта связь. Всё остальное считает
 *          через hv_feedback_volts_to_target_uV().
 */

/* -------------------------------------------------------------------------------- */
/* ------------------------------- Напряжение ------------------------------------- */
/* -------------------------------------------------------------------------------- */

/** @brief Значение верхнего плеча резистивного делителя в Омах */
#define HV_FEEDBACK_R1_Ohm (519000L) // 4x118k + 47k

/** @brief Значение нижнего плеча резистивного делителя в Омах, с учётом импеданса пина VBUS */
#define HV_FEEDBACK_R2_Ohm (76212L) // 82.5k || 1M

/** @brief Коэффициент резистивного делителя напряжения K = 1 + R1/R2 */
#define HV_FEEDBACK_DIVIDER (7.80995f) // K = 1 + R1/R2
// #define HV_FEEDBACK_DIVIDER (7.274f)

/** @brief Максимальное напряжение теста, допускаемое схемой конфигурации, В. */
#define HV_FEEDBACK_MAX_TEST_V (550UL)

/** @brief Предел шины INA239, мкВ. Соответствует BOVL_RAW из ina239.h. */
#define HV_FEEDBACK_BUS_LIMIT_uV (82500000UL)

/**
 * @brief Уставка регулятора в микровольтах на входе INA239.
 * @param volts напряжение на выходе генератора, В (как в конфигурации)
 */
static inline uint32_t hv_feedback_volts_to_target_uV (uint16_t volts) {
    return (((uint32_t)volts * 1000000UL) / HV_FEEDBACK_DIVIDER);
}

/**
 * @brief Напряжение на выходе генератора по показанию INA239, В.
 * @details Нужно только для отображения, точность здесь не важна.
 */
static inline uint32_t hv_feedback_uV_to_volts (uint32_t measured_uV) {
    return ((uint32_t)(measured_uV * HV_FEEDBACK_DIVIDER) / 1000000UL);
}

/*
 * Сборка встанет, если при выбранном коэффициенте максимальное напряжение
 * теста выйдет за предел шины INA239: значит коэффициент занижен.
 */
_Static_assert((HV_FEEDBACK_MAX_TEST_V * 1000000UL) / HV_FEEDBACK_DIVIDER 
               < HV_FEEDBACK_BUS_LIMIT_uV,
               "HV_FEEDBACK_DIVIDER занижен: максимальное напряжение теста "
               "выходит за предел BOVL INA239");

/* -------------------------------------------------------------------------------- */
/* ------------------------------------ Ток --------------------------------------- */
/* -------------------------------------------------------------------------------- */

/** @brief Проводимость шунта в микросименсах */
#define HV_SHUNT_CONDUCTANCE_uS 45455UL
/** @brief Сопротивление шунта, Ом. */
#define HV_SHUNT_RESISTANCE_Ohm 22L

/** @brief Максимальный ожидаемый ток DUT */
#define HV_SHUNT_MAX_CURRENT_nA 2000000L

/** @brief Предел показаний шунта, мкВ */
#define HV_SHUNT_LIMIT_uV 163840

/** 
 * @brief Получить ток устройства по падению напряжения на шунте
 * @details Формула I[нА] = U[мкВ] / R[кОм] = U[мкВ] * 1000 / R[Ом]
 */
static inline int32_t hv_shunt_uV_to_nA (int32_t measured_uV) {
    return (measured_uV * 1000L / HV_SHUNT_RESISTANCE_Ohm);
}

/**
 * @brief Получить падение напряжения по току устройства
 * @details Формула U[мкВ] = I[нА] * R[кОм] = I[нА] * R[Ом] / 1000
 */
static inline int32_t hv_shunt_nA_to_uV (int32_t current_nA) {
    return (current_nA * HV_SHUNT_RESISTANCE_Ohm / 1000);
}

/*
 * Сборка встанет, если при выбранном шунте максимальный ток
 * теста выйдет за предел INA239: значит значение проводимости занижено.
 */
_Static_assert(HV_SHUNT_MAX_CURRENT_nA * HV_SHUNT_RESISTANCE_Ohm / 1000 < HV_SHUNT_LIMIT_uV,
               "HV_SHUNT_RESISTANCE_Ohm завышен: максимальный ток теста "
               "выходит за предел SOVL INA239");

#endif /* HV_FEEDBACK_H_ */
