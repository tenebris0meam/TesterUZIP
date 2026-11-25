#include "states/multipole_countdown.h"

#include "display.h"
#include "input_events.h"
#include "multi_pole_test.h"
#include "proccessor.h"

void proccessor_multipole_countdown_on_enter (void) {
    multi_pole_test_start_countdown();
    return;
}

void proccessor_multipole_countdown_on_exit (void) {
    display_set_updatepolicy(DisplayUpdateSuspended);
    return;
}

void proccessor_multipole_countdown_process (void) {
    if (multi_pole_test_countdown_finished()) {
        proccessor_raise_event(CountdownComplete);
        inputevent_enable_systemevent();
    }
    return;
}

State proccessor_multipole_countdown_handle_event (Event event) {
    switch (event) {
        case CountdownComplete: return Generation;

        /* Повторный ALERT уже не является ожидаемым подтверждением. */
        case AlertActive: 
            inputevent_enable_hardwarefail();
            return HardwareFail;

        case TestDeniedByUser: 
        case ReturnToIdle: return Idle_Setting;

        default: return MultiPoleCountdown;
    }
}
