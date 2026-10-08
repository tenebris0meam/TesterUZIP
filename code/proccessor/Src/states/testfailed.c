#include "states/testfailed.h"

void proccessor_testfailed_on_enter (void) {
    proccessor_record_result(false);
    return;
}

void proccessor_testfailed_on_exit (void) {
    return;
}

void proccessor_testfailed_proccess (void) {
    proccessor_raise_after_result();
    return;
}

State proccessor_testfailed_handle_event (Event event) {
    switch (event) {
        case TestResultRecorded: return Commutation;
        case TestSequenceFinished: return TestSequenceComplete;
        case AlertActive: return HardwareFail;
        default: return TestFailed;
    }
}