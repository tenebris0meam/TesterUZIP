#include "multi_pole_test.h"

#include <stddef.h>

#define MULTI_POLE_TEST_COUNTDOWN_SECONDS (3U)
#define MULTI_POLE_TEST_COUNTDOWN_MS      (1000U)

static MultiPoleTestState state = MULTI_POLE_TEST_IDLE;
static MultiPoleTestError last_error = MULTI_POLE_TEST_ERROR_NONE;
static const MultiPoleTestOps *ops = NULL;
static uint8_t pair_index = 0U;
static uint8_t pair_count = 0U;
static uint8_t countdown_value = 0U;
static uint32_t countdown_started_ms = 0U;
static MultiPoleTestPair current_pair = { 0U, 0U };

static bool ops_are_valid(const MultiPoleTestOps *candidate)
{
    return (candidate != NULL) &&
           (candidate->get_pair != NULL) &&
           (candidate->begin_prepare != NULL) &&
           (candidate->stop_generation != NULL) &&
           (candidate->reinitialize_ina239 != NULL) &&
           (candidate->begin_test != NULL) &&
           (candidate->render != NULL) &&
           (candidate->beep != NULL) &&
           (candidate->now_ms != NULL);
}

static void render_current(void)
{
    if ((ops != NULL) && (ops->render != NULL)) {
        ops->render(state,
                    pair_index,
                    pair_count,
                    (pair_index < pair_count) ? &current_pair : NULL,
                    countdown_value,
                    last_error);
    }
}

static void enter_error(MultiPoleTestError error)
{
    /* Безопасность важнее результата callback: попытка выключить PWM всегда. */
    if (ops != NULL && ops->stop_generation != NULL) {
        (void)ops->stop_generation();
    }

    last_error = error;
    state = MULTI_POLE_TEST_ERROR;
    countdown_value = 0U;
    render_current();
}

static bool load_current_pair(void)
{
    if ((ops == NULL) || (pair_index >= pair_count)) {
        return false;
    }

    if (!ops->get_pair(pair_index, &current_pair)) {
        return false;
    }

    /* Одна и та же линия для обоих щупов почти наверняка означает ошибку config. */
    if (current_pair.red_pin == current_pair.black_pin) {
        return false;
    }

    return true;
}

static bool begin_current_prepare(void)
{
    if (!load_current_pair()) {
        enter_error(MULTI_POLE_TEST_ERROR_PAIR_UNAVAILABLE);
        return false;
    }

    if (!ops->begin_prepare(pair_index, &current_pair)) {
        enter_error(MULTI_POLE_TEST_ERROR_PREPARE_FAILED);
        return false;
    }

    state = MULTI_POLE_TEST_WAIT_FOR_SHORT;
    countdown_value = 0U;
    last_error = MULTI_POLE_TEST_ERROR_NONE;
    render_current();
    return true;
}

void multi_pole_test_init(void)
{
    /* Init itself does not touch hardware; abort is explicit and safety-aware. */
    state = MULTI_POLE_TEST_IDLE;
    last_error = MULTI_POLE_TEST_ERROR_NONE;
    ops = NULL;
    pair_index = 0U;
    pair_count = 0U;
    countdown_value = 0U;
    countdown_started_ms = 0U;
    current_pair.red_pin = 0U;
    current_pair.black_pin = 0U;
}

bool multi_pole_test_start(const MultiPoleTestOps *new_ops, uint8_t new_pair_count)
{
    if (!ops_are_valid(new_ops)) {
        ops = NULL;
        pair_count = new_pair_count;
        pair_index = 0U;
        countdown_value = 0U;
        last_error = MULTI_POLE_TEST_ERROR_BAD_ARGUMENT;
        state = MULTI_POLE_TEST_ERROR;
        return false;
    }

    if (new_pair_count == 0U) {
        ops = new_ops;
        pair_count = 0U;
        pair_index = 0U;
        countdown_value = 0U;
        enter_error(MULTI_POLE_TEST_ERROR_BAD_ARGUMENT);
        return false;
    }

    ops = new_ops;
    pair_count = new_pair_count;
    pair_index = 0U;
    countdown_value = 0U;
    last_error = MULTI_POLE_TEST_ERROR_NONE;
    state = MULTI_POLE_TEST_IDLE;

    return begin_current_prepare();
}

bool multi_pole_test_handle_alert(void)
{
    /* Only an ALERT during this exact stage is an operator confirmation. */
    if (state != MULTI_POLE_TEST_WAIT_FOR_SHORT) {
        return false;
    }

    /* BKIN should already have stopped the timer; still force software shutdown. */
    if (!ops->stop_generation()) {
        enter_error(MULTI_POLE_TEST_ERROR_GENERATION_STOP_FAILED);
        return true; /* Consumed: never fall through into normal state handling. */
    }

    /* Full reinitialization is mandatory. No fallback to flag-only clearing. */
    if (!ops->reinitialize_ina239()) {
        enter_error(MULTI_POLE_TEST_ERROR_INA239_REINIT_FAILED);
        return true;
    }

    state = MULTI_POLE_TEST_COUNTDOWN;
    countdown_started_ms = ops->now_ms();
    countdown_value = MULTI_POLE_TEST_COUNTDOWN_SECONDS;
    last_error = MULTI_POLE_TEST_ERROR_NONE;
    ops->beep();
    render_current();
    return true;
}

void multi_pole_test_process(void)
{
    if ((state != MULTI_POLE_TEST_COUNTDOWN) || (ops == NULL)) {
        return;
    }

    const uint32_t elapsed_ms = (uint32_t)(ops->now_ms() - countdown_started_ms);
    const uint32_t elapsed_seconds = elapsed_ms / MULTI_POLE_TEST_COUNTDOWN_MS;

    if (elapsed_seconds >= MULTI_POLE_TEST_COUNTDOWN_SECONDS) {
        countdown_value = 0U;
        render_current();

        if (!ops->begin_test(pair_index, &current_pair)) {
            enter_error(MULTI_POLE_TEST_ERROR_TEST_START_FAILED);
            return;
        }

        state = MULTI_POLE_TEST_RUNNING_TEST;
        render_current();
        return;
    }

    const uint8_t new_countdown =
        (uint8_t)(MULTI_POLE_TEST_COUNTDOWN_SECONDS - elapsed_seconds);

    if (new_countdown != countdown_value) {
        countdown_value = new_countdown;
        ops->beep();
        render_current();
    }
}

void multi_pole_test_test_completed(void)
{
    if (state != MULTI_POLE_TEST_RUNNING_TEST) {
        return;
    }

    /* Caller must stop generation and persist the result before this call. */
    ++pair_index;
    if (pair_index >= pair_count) {
        state = MULTI_POLE_TEST_COMPLETE;
        countdown_value = 0U;
        render_current();
        return;
    }

    (void)begin_current_prepare();
}

void multi_pole_test_abort(void)
{
    if (ops != NULL && ops->stop_generation != NULL) {
        (void)ops->stop_generation();
    }
    state = MULTI_POLE_TEST_IDLE;
    last_error = MULTI_POLE_TEST_ERROR_NONE;
    pair_index = 0U;
    pair_count = 0U;
    countdown_value = 0U;
    render_current();
    ops = NULL;
}

MultiPoleTestState multi_pole_test_get_state(void)
{
    return state;
}

MultiPoleTestError multi_pole_test_get_error(void)
{
    return last_error;
}

uint8_t multi_pole_test_get_pair_index(void)
{
    return pair_index;
}

uint8_t multi_pole_test_get_pair_count(void)
{
    return pair_count;
}

uint8_t multi_pole_test_get_countdown(void)
{
    return countdown_value;
}
