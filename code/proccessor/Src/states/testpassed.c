#include "states/testpassed.h"
#include "model.h"

void proccessor_testpassed_on_enter (void) {
    model_handle_test_result();
    return;
}

void proccessor_testpassed_on_exit (void) {
    return;
}

void proccessor_testpassed_proccess (void) {
    proccessor_raise_after_result();
    return;
}

State proccessor_testpassed_handle_event (Event event) {
    switch (event) {
        case TestResultRecorded: return Commutation;
        case TestSequenceFinished: return TestSequenceComplete;
        case AlertActive: return HardwareFail;
        default: return TestPassed;
    }
}