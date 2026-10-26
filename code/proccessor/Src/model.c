#include "model.h"
#include "calc.h"
#include "hv_feedback.h"
#include "params.h"
#include "resolver.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

TestConfig_TypeDef test_config = { 0 };
TestRuntime_TypeDef test_runtime = { 0 };
TestResult_TypeDef *test_results;



/** @brief Позиция очередной невыполненной проверки. */
static uint8_t model_next_position (void) {
    return (uint8_t)(test_runtime.passed_cnt + test_runtime.failed_cnt);
}

static uint8_t model_get_TVS_tests_num (void) {
    const Params_Device_TypeDef *device = test_runtime.current_device;
    if (device == 0) {
        return UINT8_MAX;
    }
    return device->num_seqs_type0;
}

static uint8_t model_get_VAR_tests_num (void) {
    const Params_Device_TypeDef *device = test_runtime.current_device;
    if (device == 0) {
        return UINT8_MAX;
    }
    return device->num_seqs_type1;
}

static uint8_t model_get_GDT_tests_num (void) {
    const Params_Device_TypeDef *device = test_runtime.current_device;
    if (device == 0) {
        return UINT8_MAX;
    }
    return device->num_seqs_type2;
}

bool model_alloc_test_results (const uint8_t num_tests) {
    if (num_tests >= MODEL_MAX_TESTS) {
        return false;
    }

    test_results = (TestResult_TypeDef *)malloc(num_tests * sizeof(TestResult_TypeDef));
    if (test_results) {
        memset(test_results, 0, num_tests * sizeof(TestResult_TypeDef));
        return true;
    } else {
        return false;
    }
}

void model_dealloc_test_results (void) {
    free(test_results);
    return;
}

void model_point_to (const uint8_t pos) {
    const Params_Device_TypeDef *device = test_runtime.current_device;
    if (device == 0) {
        return;
    }

    uint8_t TVS_num = device->num_seqs_type0;
    uint8_t VAR_num = device->num_seqs_type1;
    uint8_t GDT_num = device->num_seqs_type2;
    if (TVS_num && (pos < TVS_num)) {
        test_runtime.current_type = (uint8_t)TransientVoltageSupressor;
        resolver_set_TVS_VAR();
    } else if (VAR_num && (pos < TVS_num + VAR_num)) {
        test_runtime.current_type = (uint8_t)Varistor;
        resolver_set_TVS_VAR();
    } else if (GDT_num && (pos < TVS_num + VAR_num + GDT_num)) {
        test_runtime.current_type = (uint8_t)GasDischargeTube;
        resolver_set_GDT();
    }

    test_runtime.current_sequence = model_get_sequence_at(pos, (const ResolveType)test_runtime.current_type);
    test_runtime.current_test_params = model_get_testparams_at(pos, (const ResolveType)test_runtime.current_type);
    return;
}

const Params_TestSequence_TypeDef *model_get_sequence_at (const uint8_t pos, const ResolveType type) {
    const Params_Device_TypeDef *device = test_runtime.current_device;
    if (device == 0) {
        return 0;
    }

    bool check_pos, check_seq;
    uint8_t pos_without_offset;
    switch (type) {
        case TransientVoltageSupressor: 
            pos_without_offset = pos;
            check_pos = (pos >= device->num_seqs_type0);
            check_seq = (device->test_seqs_type0 == 0);
            break;
        case Varistor: 
            pos_without_offset = pos - device->num_seqs_type0;
            check_pos = (pos_without_offset >= device->num_seqs_type1);
            check_seq = (device->test_seqs_type1 == 0);
            break;
        case GasDischargeTube: 
            pos_without_offset = pos - (device->num_seqs_type0 + device->num_seqs_type1);
            check_pos = (pos_without_offset >= device->num_seqs_type2);
            check_seq = (device->test_seqs_type2 == 0);
            break;
        default: 
            check_pos = true;
            check_seq = true;
            break;
    }
    if (check_pos || check_seq) {
        return 0;
    }

    uint8_t seq_idx;
    switch (type) {
        case TransientVoltageSupressor: seq_idx = device->test_seqs_type0[pos_without_offset].seq_idx; break;
        case Varistor: seq_idx = device->test_seqs_type1[pos_without_offset].seq_idx; break;
        case GasDischargeTube: seq_idx = device->test_seqs_type2[pos_without_offset].seq_idx; break;
        default: seq_idx = UINT8_MAX; break;
    }

    return params_get_sequence(&params, seq_idx);
}

const Params_TestParam_TypeDef *model_get_testparams_at (const uint8_t pos, const ResolveType type) {
    const Params_Device_TypeDef *device = test_runtime.current_device;
    if (device == 0) {
        return 0;
    }

    bool check_pos, check_seq;
    uint8_t pos_without_offset;
    switch (type) {
        case TransientVoltageSupressor: 
            pos_without_offset = pos;
            check_pos = (pos_without_offset >= device->num_seqs_type0);
            check_seq = (device->test_seqs_type0 == 0);
            break;
        case Varistor: 
            pos_without_offset = pos - device->num_seqs_type0;
            check_pos = (pos_without_offset >= device->num_seqs_type1);
            check_seq = (device->test_seqs_type1 == 0);
            break;
        case GasDischargeTube: 
            pos_without_offset = pos - (device-> num_seqs_type0 + device->num_seqs_type1);
            check_pos = (pos_without_offset >= device->num_seqs_type2);
            check_seq = (device->test_seqs_type2 == 0);
            break;
        default: 
            check_pos = true;
            check_seq = true;
            break;
    }
    if (check_pos || check_seq) {
        return 0;
    }

    const Params_TestParam_TypeDef *testparam;
    switch (type) {
        case TransientVoltageSupressor: 
            testparam = 
                params_get_test_params_for_TVS(&params, 
                                               device->test_seqs_type0[pos_without_offset].test_param_idx);
            break;
        case Varistor: 
            testparam = 
                params_get_test_params_for_VAR(&params, 
                                               device->test_seqs_type1[pos_without_offset].test_param_idx);
            break;
        case GasDischargeTube: 
            testparam = 
                params_get_test_params_for_GDT(&params, 
                                               device->test_seqs_type2[pos_without_offset].test_param_idx);
            break;
        default: testparam = 0; break;
    }

    return testparam;
}

uint8_t model_get_total_tests (void) {
    const Params_Device_TypeDef *device = test_runtime.current_device;
    if (device == 0) {
        return 0;
    }
    uint8_t general_num_seqs = device->num_seqs_type0 + 
                               device->num_seqs_type1 + 
                               device->num_seqs_type2;
    return (uint8_t)((general_num_seqs > MODEL_MAX_TESTS) ? MODEL_MAX_TESTS : general_num_seqs);
}

void model_reset_results (void) {
    test_runtime.done_mask = 0;
    test_runtime.pass_mask = 0;
    test_runtime.passed_cnt = 0;
    test_runtime.failed_cnt = 0;
    test_runtime.fail_was = false;
    model_point_to(0);
    model_dealloc_test_results();
    return;
}

void model_select_device (const uint8_t device_idx) {
    test_runtime.current_device = params_get_device(&params, device_idx);
    test_config.device_id = device_idx;
    if (test_runtime.current_device != 0) {
        test_config.series_id = test_runtime.current_device->seria_idx;
        test_config.group_id = test_runtime.current_device->group_idx;
    }
    model_reset_results();
    return;
}

void model_remember_result (const ResolveType type) {
    uint8_t cur_idx = test_runtime.passed_cnt + test_runtime.failed_cnt;
    const CalcMetrics_TypeDef *cur_metrics = calc_get_current_metrics();
    memcpy(&test_results[cur_idx].res, cur_metrics, sizeof(CalcMetrics_TypeDef));
    test_results[cur_idx].type = (uint8_t)type;
    return;
}

void model_change_mode (const Modes_TypeDef new_mode) {
    if (new_mode >= Modes_Count) {
        return;
    }
    test_config.mode_id = new_mode;
    return;
}

void model_init (void) {
    model_select_device(0);
    model_change_mode(CheckDevice);
    return;
}

void model_handle_test_result (void) {
    uint8_t pos = test_runtime.passed_cnt + test_runtime.failed_cnt;
    if (pos >= MODEL_MAX_TESTS) {
        return;
    }
    if (pos >= model_get_total_tests()) {
        return;
    }

    uint32_t bit = (uint32_t)1UL << pos;
    if (test_runtime.done_mask & bit) {
        /* Повторный результат по той же позиции не засчитываем дважды */
        return;
    }
    test_runtime.done_mask |= bit;

    const Params_TestParam_TypeDef *testparams = test_runtime.current_test_params;
    int32_t voltage_lowth_uV = hv_feedback_volts_to_target_uV(testparams->min_voltage);
    int32_t voltage_highth_uV = hv_feedback_volts_to_target_uV(testparams->max_voltage);
    int32_t voltage_lower_uV = test_results[pos].res.voltage_uV.mean_value - 3 * test_results[pos].res.voltage_uV.standard_deviation;
    int32_t voltage_higher_uV = test_results[pos].res.voltage_uV.mean_value + 3 * test_results[pos].res.voltage_uV.standard_deviation;
    bool is_voltage_within_range = ((voltage_lower_uV > voltage_lowth_uV) && 
                                    (voltage_higher_uV < voltage_highth_uV));
    bool is_current_within_range = ((test_results[pos].res.current_nA.mean_value > CALC_CURRENT_LOW_nA) && 
                                    (test_results[pos].res.current_nA.mean_value < CALC_CURRENT_HIGH_nA));
    if (is_voltage_within_range) {
        // тест пройден
        if (test_results[pos].type != GasDischargeTube) {
            // тест не для разрядника, ток тоже проверяем
            if (is_current_within_range) {
                test_results[pos].pass = true;
            } else {
                test_results[pos].pass = false;
            }
        } else {
            // тест для разрядника, проверяем только напряжение ионизации и факт наличия броска тока
            if (test_results[pos].res.current_nA.mean_value == INT32_MAX) {
                test_results[pos].pass = true;
            } else {
                test_results[pos].pass = false;
            }
        }
    } else {
        // тест не пройден
        test_results[pos].pass = false;
    }

    if (test_results[pos].pass) {
        test_runtime.pass_mask |= bit;
        test_runtime.passed_cnt++;
    } else {
        test_runtime.fail_was = true;
        test_runtime.failed_cnt++;
    }

    /* Переходим к следующей проверке; если она была последней - сбрасываем */
    model_point_to(model_next_position());
    return;
}

bool model_is_test_done (const uint8_t pos) {
    if (pos >= MODEL_MAX_TESTS) {
        return false;
    }
    return (bool)((test_runtime.done_mask >> pos) & 1UL);
}

bool model_is_test_passed (const uint8_t pos) {
    if (pos >= MODEL_MAX_TESTS) {
        return false;
    }
    return (bool)((test_runtime.pass_mask >> pos) & 1UL);
}

bool model_is_test_failed (const uint8_t pos) {
    return (model_is_test_done(pos) && !model_is_test_passed(pos));
}
