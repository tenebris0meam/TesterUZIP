#include "states/multipole_countdown.h"

#include "multi_pole_test.h"

void proccessor_multipole_countdown_on_enter(void)
{
    multi_pole_test_start_countdown();
}

void proccessor_multipole_countdown_on_exit(void)
{
}

void proccessor_multipole_countdown_process(void)
{
    if (multi_pole_test_countdown_finished()) {
        proccessor_raise_event(CountdownComplete);
    }
}

State proccessor_multipole_countdown_handle_event(Event event)
{
    switch (event) {
        case CountdownComplete:
            return Generation;

        case AlertActive:
            /* Повторный ALERT уже не является ожидаемым подтверждением. */
            return HardwareFail;

        case ReturnToIdle:
            return Idle_Setting;

        default:
            return MultiPoleCountdown;
    }
}
