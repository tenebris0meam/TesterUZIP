#include "model.h"
#include "params.h"

#include <stdint.h>
#include <stdbool.h>

TestConfig_TypeDef test_config = { 0 };
TestRuntime_TypeDef test_results = { 0 };



/** @brief Позиция очередной невыполненной проверки. */
static uint8_t model_next_position (void) {
    return (uint8_t)(test_results.passed_cnt + test_results.failed_cnt);
}

static uint8_t model_get_TVS_tests_num (void) {
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return UINT8_MAX;
    }
    return device->num_seqs_type0;
}

static uint8_t model_get_VAR_tests_num (void) {
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return UINT8_MAX;
    }
    return device->num_seqs_type1;
}

static uint8_t model_get_GDT_tests_num (void) {
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return UINT8_MAX;
    }
    return device->num_seqs_type2;
}

void model_point_to (uint8_t pos) {
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return;
    }

    uint8_t TVS_num = device->num_seqs_type0;
    uint8_t VAR_num = device->num_seqs_type1;
    uint8_t GDT_num = device->num_seqs_type2;
    if (TVS_num && (pos < TVS_num)) {
        test_results.current_type = TransientVoltageSupressor;
        resolver_set_TVS_VAR();
    } else if (VAR_num && (pos < TVS_num + VAR_num)) {
        test_results.current_type = Varistor;
        resolver_set_TVS_VAR();
    } else if (GDT_num && (pos < TVS_num + VAR_num + GDT_num)) {
        test_results.current_type = GasDischargeTube;
        resolver_set_GDT();
    }

    test_results.current_sequence = model_get_sequence_at(pos, test_results.current_type);
    test_results.current_test_params = model_get_testparams_at(pos, test_results.current_type);
    return;
}

const Params_TestSequence_TypeDef *model_get_sequence_at (uint8_t pos, ResolveType type) {
    const Params_Device_TypeDef *device = test_results.current_device;
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

const Params_TestParam_TypeDef *model_get_testparams_at (uint8_t pos, ResolveType type) {
    const Params_Device_TypeDef *device = test_results.current_device;
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
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return 0;
    }
    uint8_t general_num_seqs = device->num_seqs_type0 + 
                               device->num_seqs_type1 + 
                               device->num_seqs_type2;
    return (uint8_t)((general_num_seqs > MODEL_MAX_TESTS) ? MODEL_MAX_TESTS : general_num_seqs);
}

void model_reset_results (void) {
    test_results.done_mask = 0;
    test_results.pass_mask = 0;
    test_results.passed_cnt = 0;
    test_results.failed_cnt = 0;
    test_results.fail_was = false;
    model_point_to(0);
    return;
}

void model_select_device (uint8_t device_idx) {
    test_results.current_device = params_get_device(&params, device_idx);
    test_config.device_id = device_idx;
    if (test_results.current_device != 0) {
        test_config.series_id = test_results.current_device->seria_idx;
        test_config.group_id = test_results.current_device->group_idx;
    }
    model_reset_results();
    return;
}

void model_change_mode (Modes_TypeDef new_mode) {
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

void model_handle_test_result (const TestResult_TypeDef *result) {
    if (result == 0) {
        return;
    }

    uint8_t pos = result->sequence_pos;
    if (pos >= MODEL_MAX_TESTS) {
        return;
    }
    if (pos >= model_get_total_tests()) {
        return;
    }

    uint32_t bit = (uint32_t)1UL << pos;
    if (test_results.done_mask & bit) {
        /* Повторный результат по той же позиции не засчитываем дважды */
        return;
    }
    test_results.done_mask |= bit;

    if (result->pass) {
        test_results.pass_mask |= bit;
        test_results.passed_cnt++;
    } else {
        test_results.fail_was = true;
        test_results.failed_cnt++;
    }

    /* Переходим к следующей проверке; если она была последней - сбрасываем */
    model_point_to(model_next_position());
    return;
}

bool model_is_test_done (uint8_t pos) {
    if (pos >= MODEL_MAX_TESTS) {
        return false;
    }
    return (bool)((test_results.done_mask >> pos) & 1UL);
}

bool model_is_test_passed (uint8_t pos) {
    if (pos >= MODEL_MAX_TESTS) {
        return false;
    }
    return (bool)((test_results.pass_mask >> pos) & 1UL);
}

bool model_is_test_failed (uint8_t pos) {
    return (model_is_test_done(pos) && !model_is_test_passed(pos));
}
