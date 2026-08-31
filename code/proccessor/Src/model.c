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

/** @brief Навести current_sequence на проверку с указанной позицией. */
static void model_point_to (uint8_t pos) {
    test_results.current_sequence = model_get_sequence_at(pos);
    return;
}

const Params_TestSequence_TypeDef *model_get_sequence_at (uint8_t pos) {
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return 0;
    }

    if (pos >= device->num_seqs) {
        return 0;
    }
    if (device->sequence_idxs == 0) {
        return 0;
    }

    return params_get_sequence(&params, device->sequence_idxs[pos]);
}

uint8_t model_get_total_tests (void) {
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return 0;
    }

    return (uint8_t)((device->num_seqs > MODEL_MAX_TESTS) ? MODEL_MAX_TESTS : device->num_seqs);
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

void model_init (void) {
    model_select_device(0);
    test_config.interface_id = Probe;
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
    uint8_t next = model_next_position();
    if (next < model_get_total_tests()) {
        model_point_to(next);
    } else {
        test_results.current_sequence = 0;
    }
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
