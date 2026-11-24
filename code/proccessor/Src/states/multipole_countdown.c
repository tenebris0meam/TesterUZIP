#include "states/multipole_countdown.h"

#include "display.h"
#include "multi_pole_test.h"
#include "proccessor.h"

void proccessor_multipole_countdown_on_enter (void) {
    multi_pole_test_start_countdown();
    display_set_screen(ScreenMultiPoleCountdown);
    return;
}

void proccessor_multipole_countdown_on_exit (void) {
    display_deny_render();
    return;
}

void proccessor_multipole_countdown_process (void) {
    if (multi_pole_test_countdown_finished()) {
        proccessor_raise_event(CountdownComplete);
    }
    return;
}

State proccessor_multipole_countdown_handle_event (Event event) {
    switch (event) {
        case CountdownComplete:
            /* костыль: надо запретить рендер при переходе из MultiPoleCountdown в Generation */
            display_set_screen(ScreenTestRunning);
            display_routine();
            return Generation;

        case AlertActive:
            /* Повторный ALERT уже не является ожидаемым подтверждением. */
            return HardwareFail;

        case ReturnToIdle: return Idle_Setting;

        default: return MultiPoleCountdown;
    }
}
