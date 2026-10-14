#ifndef PARAMS_H_
#define PARAMS_H_

#include <stdint.h>

/**
 * @file params.h
 * @brief Модель конфигурации тестера.
 *
 * @details Все структуры конфигурации лежат во Flash и генерируются на этапе
 *          сборки скриптом tools/minify_json.py из config/new_config.json.
 *          В ОЗУ не копируется ничего: params - это const-структура во Flash,
 *          а имена представлены парой указателей MyString внутрь пула строк,
 *          который лежит там же.
 */

#define MAX_NAME_SIZE (31)

/** @brief Сигнатура блока конфигурации: 'T','U','Z','P'. */
#define CONFIG_MAGIC (0x505A5554UL)

/** @brief Версия формата блока конфигурации. Поднимать при смене раскладки. */
#define CONFIG_VERSION (1UL)

typedef enum {
    Probe, 
    RJ45, 
    K1 
} Param_Connector;

/**
 * @brief Ссылка на строку во Flash.
 * @details Не завершена нулём: строка задана полуинтервалом [beg, end).
 */
typedef struct {
    const char *beg;
    const char *end;
} MyString;



static inline uint32_t MyString_GetSize (const MyString *const str) {
    return ((uint32_t)(str->end - str->beg));
}



/** @brief Одна тестовая последовательность: пара коммутируемых линий. */
typedef struct {
    uint8_t input_pin;
    uint8_t output_pin;
} Params_TestSequence_TypeDef;

typedef struct {
    uint16_t min_voltage;
    uint16_t max_voltage;
} Params_TestParam_TypeDef;

typedef struct {
    uint8_t seq_idx;
    uint8_t test_param_idx;
} Params_TestSeq_TypeDef;

typedef struct {
    uint8_t begin_idx;
    uint8_t count;
} Params_Range_TypeDef;

/** @brief Описание одного испытуемого прибора. */
typedef struct {
    uint8_t seria_idx;
    uint8_t group_idx;
    uint8_t device_idx;
    const Params_TestSeq_TypeDef *test_seqs_type0;
    uint8_t num_seqs_type0;
    const Params_TestSeq_TypeDef *test_seqs_type1;
    uint8_t num_seqs_type1;
    const Params_TestSeq_TypeDef *test_seqs_type2;
    uint8_t num_seqs_type2;
    uint8_t connector_id;
} Params_Device_TypeDef;

/**
 * @brief Корень конфигурации.
 * @details Массивы, на которые ссылаются поля, лежат во Flash сразу за
 *          структурой. Указатели - одинарные: таблиц указателей нет,
 *          индексация идёт прямо по массиву.
 */
typedef struct {
    uint32_t magic;                 /**< CONFIG_MAGIC   */
    uint32_t version;               /**< CONFIG_VERSION */
    uint8_t series_count;
    uint8_t groups_count;
    uint8_t devices_count;
    uint8_t sequences_count;
    uint8_t test_params_count0;
    uint8_t test_params_count1;
    uint8_t test_params_count2;
    const MyString *series_names;
    const MyString *groups_names;
    const MyString *devices_names;
    const Params_TestSequence_TypeDef *sequences;
    const Params_TestParam_TypeDef *test_params_type0;
    const Params_TestParam_TypeDef *test_params_type1;
    const Params_TestParam_TypeDef *test_params_type2;
    const Params_Range_TypeDef *series_groups;
    const Params_Range_TypeDef *group_devices;
    const Params_Device_TypeDef *devices;
} Params_TypeDef;



/**
 * @brief Сгенерированная конфигурация во Flash.
 * @details Определена в сгенерированном config_data.c, секция .config.
 */
extern const Params_TypeDef params;



static inline uint8_t params_get_series_num (const Params_TypeDef *cfg) {
    return cfg->series_count;
}
static inline uint8_t params_get_groups_num (const Params_TypeDef *cfg) {
    return cfg->groups_count;
}
static inline uint8_t params_get_devices_num (const Params_TypeDef *cfg) {
    return cfg->devices_count;
}
static inline uint8_t params_get_sequences_num (const Params_TypeDef *cfg) {
    return cfg->sequences_count;
}
static inline uint8_t params_get_testparams_num_for_TVS (const Params_TypeDef *cfg) {
    return cfg->test_params_count0;
}
static inline uint8_t params_get_testparams_num_for_VAR (const Params_TypeDef *cfg) {
    return cfg->test_params_count1;
}
static inline uint8_t params_get_testparams_num_for_GDT (const Params_TypeDef *cfg) {
    return cfg->test_params_count2;
}

static inline 
const MyString *params_get_seria_name (const Params_TypeDef *cfg, 
                                       uint8_t ser_idx) {
    return ((ser_idx < cfg->series_count) ? 
            &cfg->series_names[ser_idx] : 
            0);
}

static inline 
const MyString *params_get_group_name (const Params_TypeDef *cfg, 
                                       uint8_t grp_idx) {
    return ((grp_idx < cfg->groups_count) ? 
            &cfg->groups_names[grp_idx] : 
            0);
}

static inline 
const MyString *params_get_device_name (const Params_TypeDef *cfg, 
                                        uint8_t dev_idx) {
    return ((dev_idx < cfg->devices_count) ? 
            &cfg->devices_names[dev_idx] : 
            0);
}

static inline 
const Params_TestSequence_TypeDef *params_get_sequence (const Params_TypeDef *cfg, 
                                                        uint8_t seq_idx) {
    return ((seq_idx < cfg->sequences_count) ? 
            &cfg->sequences[seq_idx] : 
            0);
}

static inline 
const Params_Device_TypeDef *params_get_device (const Params_TypeDef *cfg, 
                                                uint8_t dev_idx) {
    return ((dev_idx < cfg->devices_count) ? 
            &cfg->devices[dev_idx] : 
            0);
}

static inline 
const Params_Range_TypeDef *params_get_groups_in_seria (const Params_TypeDef *cfg, 
                                                        uint8_t seria_idx) {
    return ((seria_idx < cfg->series_count) ? 
            &cfg->series_groups[seria_idx] : 
            0);
}

static inline 
const Params_Range_TypeDef *params_get_devices_in_group (const Params_TypeDef *cfg, 
                                                         uint8_t group_idx) {
    return ((group_idx < cfg->groups_count) ? 
            &cfg->group_devices[group_idx] : 
            0);
}

static inline
const Params_TestParam_TypeDef *params_get_test_params_for_TVS (const Params_TypeDef *cfg, 
                                                                uint8_t test_idx) {
    return ((test_idx < cfg->test_params_count0) ? 
            &cfg->test_params_type0[test_idx] : 
            0);
}

static inline
const Params_TestParam_TypeDef *params_get_test_params_for_VAR (const Params_TypeDef *cfg, 
                                                                uint8_t test_idx) {
    return ((test_idx < cfg->test_params_count1) ? 
            &cfg->test_params_type1[test_idx] : 
            0);
}

static inline
const Params_TestParam_TypeDef *params_get_test_params_for_GDT (const Params_TypeDef *cfg, 
                                                                uint8_t test_idx) {
    return ((test_idx < cfg->test_params_count2) ? 
            &cfg->test_params_type2[test_idx] : 
            0);
}



#endif /* PARAMS_H_ */
