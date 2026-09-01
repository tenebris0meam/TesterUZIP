#ifndef HV_FEEDBACK_H_
#define HV_FEEDBACK_H_

#include <stdint.h>

/**
 * @file hv_feedback.h
 * @brief Связь между напряжением на выходе генератора и тем, что видит INA239.
 *
 * @details ЭТО ЕДИНСТВЕННОЕ МЕСТО, где живёт эта связь. Всё остальное считает
 *          через hv_feedback_volts_to_target_uV().
 */
// #if defined DEBUG
// #define HV_FEEDBACK_DIVIDER (5.994f)

// /** @brief Максимальное напряжение теста, допускаемое схемой конфигурации, В. */
// #define HV_FEEDBACK_MAX_TEST_V (400UL)
// #else 
#define HV_FEEDBACK_DIVIDER (7.267f)

/** @brief Максимальное напряжение теста, допускаемое схемой конфигурации, В. */
#define HV_FEEDBACK_MAX_TEST_V (550UL)
// #endif

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

#endif /* HV_FEEDBACK_H_ */
