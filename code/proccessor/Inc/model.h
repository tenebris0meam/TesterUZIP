#ifndef MODEL_H_
#define MODEL_H_

#include "params.h"

#include <stdint.h>
#include <stdbool.h>

/**
 * @file model.h
 * @brief Состояние текущей проверки прибора.
 */

/** @brief Разрядность масок результатов: столько позиций проверки помещается. */
#define MODEL_MAX_TESTS (32)

/** @brief Что выбрано пользователем на экранах настроек. */
typedef struct {
    Param_Connector interface_id;
    uint8_t series_id;
    uint8_t group_id;
    uint8_t device_id;
} TestConfig_TypeDef;

/** @brief Результат одной проверки, передаётся в model_handle_test_result(). */
typedef struct {
    uint8_t sequence_pos;       /**< позиция проверки, 0..MODEL_MAX_TESTS-1 */
    uint32_t device_voltage;
    uint32_t device_current;
    bool pass;
} TestResult_TypeDef;

/** @brief Ход текущей проверки прибора. */
typedef struct {
    const Params_TestSequence_TypeDef *current_sequence;
    const Params_Device_TypeDef *current_device;
    uint32_t done_mask;         /**< бит i: проверка i выполнена  */
    uint32_t pass_mask;         /**< бит i: проверка i пройдена   */
    uint8_t passed_cnt;
    uint8_t failed_cnt;
    bool fail_was;
} TestRuntime_TypeDef;

extern TestConfig_TypeDef test_config;
extern TestRuntime_TypeDef test_results;

/** @brief Выбрать первый прибор конфигурации и очистить результаты. */
void model_init (void);

/** @brief Сменить проверяемый прибор и начать проверку заново. */
void model_select_device (uint8_t device_idx);

/** @brief Сбросить результаты, оставив выбранный прибор. */
void model_reset_results (void);

/** @brief Засчитать результат очередной проверки и перейти к следующей. */
void model_handle_test_result (const TestResult_TypeDef *result);

/** @brief Сколько всего проверок у выбранного прибора. */
uint8_t model_get_total_tests (void);

/** @brief Выполнена ли проверка с указанной позицией. */
bool model_is_test_done (uint8_t pos);

/** @brief Пройдена ли проверка с указанной позицией. */
bool model_is_test_passed (uint8_t pos);

/** @brief Провалена ли проверка с указанной позицией (выполнена и не пройдена). */
bool model_is_test_failed (uint8_t pos);

/** @brief Тестовая последовательность, соответствующая позиции проверки. */
const Params_TestSequence_TypeDef *model_get_sequence_at (uint8_t pos);

#endif /* MODEL_H_ */
