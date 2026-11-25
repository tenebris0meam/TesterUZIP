#include "states/testsequence_complete.h"
#include "display.h"
#include "input_events.h"
#include "buzzer.h"

void proccessor_testsequencecomplete_on_enter (void) {
    /* Экран сам перейдёт на итоги: событие увидит inputevent_routine() */
    inputevent_testing_completed();
    buzzer_turn_on();
    display_set_updatepolicy(DisplayUpdateEnable);
    return;
}

void proccessor_testsequencecomplete_on_exit (void) {
    return;
}

void proccessor_testsequencecomplete_proccess (void) {
    proccessor_raise_event(ReturnToIdle);
    return;
}

State proccessor_testsequencecomplete_handle_event (Event event) {
    switch (event) {
        case ReturnToIdle: return Idle_Setting;
        case AlertActive: 
            inputevent_enable_hardwarefail();
            return HardwareFail;
        default: return TestSequenceComplete;
    }
}