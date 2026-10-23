#ifndef MODEL_H_
#define MODEL_H_

#include "calc.h"
#include "params.h"
#include "resolver.h"

#include <stdint.h>
#include <stdbool.h>

/**
 * @file model.h
 * @brief Состояние текущей проверки прибора.
 */

/** @brief Разрядность масок результатов: столько позиций проверки помещается. */
#define MODEL_MAX_TESTS (32)

#define MAX_TESTS_FOR_ONE_GDT 3

/** @brief Что выбрано пользователем на экранах настроек. */
typedef struct {
    uint8_t series_id;
    uint8_t group_id;
    uint8_t device_id;
    Modes_TypeDef mode_id;
} TestConfig_TypeDef;

/** @brief Результат одной проверки, передаётся в model_handle_test_result(). */
typedef struct {
    CalcMetrics_TypeDef res;
    uint8_t sequence_pos;       /**< позиция проверки, 0..MODEL_MAX_TESTS-1 */
    bool pass;
} TestResult_TypeDef;

/** @brief Ход текущей проверки прибора. */
typedef struct {
    const Params_TestSequence_TypeDef *current_sequence;
    const Params_TestParam_TypeDef *current_test_params;
    const Params_Device_TypeDef *current_device;
    uint32_t done_mask;         /**< бит i: проверка i выполнена  */
    uint32_t pass_mask;         /**< бит i: проверка i пройдена   */

    /** @ref ResolveType */
    uint8_t current_type; 
    uint8_t passed_cnt;
    uint8_t failed_cnt;
    bool fail_was;
} TestRuntime_TypeDef;

extern TestConfig_TypeDef test_config;
extern TestRuntime_TypeDef test_runtime;
extern TestResult_TypeDef *test_results;

/** @brief Выбрать первый прибор конфигурации и очистить результаты. */
void model_init (void);

/** @brief Сменить проверяемый прибор и начать проверку заново. */
void model_select_device (const uint8_t device_idx);

/** @brief Сменить режим работы устройства */
void model_change_mode (const Modes_TypeDef new_mode);

/** @brief Сбросить результаты, оставив выбранный прибор. */
void model_reset_results (void);

/** @brief Засчитать результат очередной проверки и перейти к следующей. */
void model_handle_test_result (void);

/** @brief Сколько всего проверок у выбранного прибора. */
uint8_t model_get_total_tests (void);

/** @brief Выполнена ли проверка с указанной позицией. */
bool model_is_test_done (const uint8_t pos);

/** @brief Пройдена ли проверка с указанной позицией. */
bool model_is_test_passed (const uint8_t pos);

/** @brief Провалена ли проверка с указанной позицией (выполнена и не пройдена). */
bool model_is_test_failed (const uint8_t pos);

/** @brief Тестовая последовательность, соответствующая позиции проверки. */
const Params_TestSequence_TypeDef *model_get_sequence_at (const uint8_t pos, const ResolveType type);

/** @brief Тестовые параметры, соответствующая позиции проверки. */
const Params_TestParam_TypeDef *model_get_testparams_at (const uint8_t pos, const ResolveType type);

/** @brief Навести current_sequence на проверку с указанной позицией. */
void model_point_to (const uint8_t pos);

bool model_alloc_test_results (const uint8_t num_tests);

void model_dealloc_test_results (void);

void model_remember_result (void);

#endif /* MODEL_H_ */
