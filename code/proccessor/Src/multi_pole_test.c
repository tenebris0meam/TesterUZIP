#include "multi_pole_test.h"
#include "stm32f103xb.h"
#include "system_stm32f1xx.h"

static uint32_t countdown_started_cycles;
static bool countdown_started;

void multi_pole_test_start_countdown(void)
{
    countdown_started_cycles = DWT->CYCCNT;
    countdown_started = true;
}

uint8_t multi_pole_test_countdown_remaining(void)
{
    if (!countdown_started) {
        return 0U;
    }

    const uint32_t elapsed_cycles = DWT->CYCCNT - countdown_started_cycles;
    const uint32_t elapsed_seconds = elapsed_cycles / SystemCoreClock;

    if (elapsed_seconds >= MULTI_POLE_TEST_COUNTDOWN_SECONDS) {
        return 0U;
    }

    return (uint8_t)(MULTI_POLE_TEST_COUNTDOWN_SECONDS - elapsed_seconds);
}

bool multi_pole_test_countdown_finished(void)
{
    return countdown_started && (multi_pole_test_countdown_remaining() == 0U);
}
