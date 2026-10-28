#include "resolver.h"
#include "proccessor.h"
#include "states/generation.h"
#include "states/hardware_fail.h"

/* -------------------------------------------------------------------- */
/* ----------------------- Public Functions --------------------------- */
/* -------------------------------------------------------------------- */

void resolver_set_GDT (void) {
    proccessor[Generation].proccess = proccessor_generation_proccess_checkdevice_for_GDT;
    proccessor[Generation].handle_event = proccessor_generation_handle_event_checkdevice_for_GDT;
    proccessor[Generation].on_exit = proccessor_generation_on_exit_checkdevice_for_GDT;
    proccessor[HardwareFail].on_enter = proccessor_hardwarefail_on_enter_for_GDT;
    proccessor[HardwareFail].proccess = proccessor_hardwarefail_proccess_for_GDT;
    proccessor[HardwareFail].on_exit = proccessor_hardwarefail_on_exit_for_GDT;
    proccessor[HardwareFail].handle_event = proccessor_hardwarefail_handle_event_for_GDT;
    return;
}

void resolver_set_TVS_VAR (void) {
    proccessor[Generation].proccess = proccessor_generation_proccess_checkdevice_default;
    proccessor[Generation].handle_event = proccessor_generation_handle_event_checkdevice_default;
    proccessor[Generation].on_exit = proccessor_generation_on_exit_checkdevice_default;
    proccessor[HardwareFail].on_enter = proccessor_hardwarefail_on_enter_default;
    proccessor[HardwareFail].proccess = proccessor_hardwarefail_proccess_default;
    proccessor[HardwareFail].on_exit = proccessor_hardwarefail_on_exit_default;
    proccessor[HardwareFail].handle_event = proccessor_hardwarefail_handle_event_default;
    return;
}

#ifdef TESTER_MODES

#include "commutation.h"
#include "testfailed.h"
#include "testpassed.h"
#include "testsequence_complete.h"

void resolver_set_new_mode (Modes_TypeDef new_mode) {
    switch (new_mode) {
        case CheckDevice: 
            proccessor[Commutation].on_enter = proccessor_commutation_on_enter_checkdevice; 
            proccessor[Commutation].on_exit = proccessor_commutation_on_exit;
            proccessor[Commutation].proccess = proccessor_commutation_proccess_default;
            proccessor[Commutation].handle_event = proccessor_commutation_handle_event_default;
            proccessor[Generation].on_enter = proccessor_generation_on_enter_checkdevice;
            proccessor[Generation].on_exit = proccessor_generation_on_exit_checkdevice_default;
            proccessor[Generation].proccess = proccessor_generation_proccess_checkdevice_default;
            proccessor[Generation].handle_event = proccessor_generation_handle_event_checkdevice_default;
            proccessor[TestPassed].on_enter = proccessor_testpassed_on_enter;
            proccessor[TestPassed].on_exit = proccessor_testpassed_on_exit;
            proccessor[TestPassed].proccess = proccessor_testpassed_proccess;
            proccessor[TestPassed].handle_event = proccessor_testpassed_handle_event;
            proccessor[TestFailed].on_enter = proccessor_testfailed_on_enter;
            proccessor[TestFailed].on_exit = proccessor_testfailed_on_exit;
            proccessor[TestSequenceComplete].on_enter = proccessor_testsequencecomplete_on_enter;
            proccessor[TestSequenceComplete].on_exit = proccessor_testsequencecomplete_on_exit;
            proccessor[TestSequenceComplete].proccess = proccessor_testsequencecomplete_proccess;
            proccessor[TestSequenceComplete].handle_event = proccessor_testsequencecomplete_handle_event;
            proccessor[HardwareFail].on_enter = proccessor_hardwarefail_on_enter_default;
            proccessor[HardwareFail].on_exit = proccessor_hardwarefail_on_exit_default;
            proccessor[HardwareFail].proccess = proccessor_hardwarefail_proccess_default;
            proccessor[HardwareFail].handle_event = proccessor_hardwarefail_handle_event_default;
            return;

        case Voltmeter:
            proccessor[HardwareFail].on_enter = proccessor_hardwarefail_on_enter_default;
            proccessor[HardwareFail].on_exit = proccessor_hardwarefail_on_exit_default;
            proccessor[HardwareFail].proccess = proccessor_hardwarefail_proccess_default;
            proccessor[HardwareFail].handle_event = proccessor_hardwarefail_handle_event_default;
            proccessor[Commutation].on_enter = proccessor_commutation_on_enter_probe;
            proccessor[Commutation].on_exit = proccessor_commutation_on_exit;
            proccessor[Commutation].proccess = proccessor_commutation_proccess_default;
            proccessor[Commutation].handle_event = proccessor_commutation_handle_event_default;
            proccessor[Generation].on_enter = proccessor_generation_on_enter_voltmeter;
            proccessor[Generation].on_exit = proccessor_generation_on_exit_voltmeter;
            proccessor[Generation].proccess = proccessor_generation_proccess_voltmeter;
            proccessor[Generation].handle_event = proccessor_generation_handle_event_voltmeter;
            return;

        case Ommeter:
            proccessor[HardwareFail].on_enter = proccessor_hardwarefail_on_enter_default;
            proccessor[HardwareFail].on_exit = proccessor_hardwarefail_on_exit_default;
            proccessor[HardwareFail].proccess = proccessor_hardwarefail_proccess_default;
            proccessor[HardwareFail].handle_event = proccessor_hardwarefail_handle_event_default;
            proccessor[Commutation].on_enter = proccessor_commutation_on_enter_probe; 
            proccessor[Commutation].on_exit = proccessor_commutation_on_exit;
            proccessor[Commutation].proccess = proccessor_commutation_proccess_default;
            proccessor[Commutation].handle_event = proccessor_commutation_handle_event_default;
            proccessor[Generation].on_enter = proccessor_generation_on_enter_ommeter;
            proccessor[Generation].on_exit = proccessor_generation_on_exit_ommeter;
            proccessor[Generation].proccess = proccessor_generation_proccess_ommeter;
            proccessor[Generation].handle_event = proccessor_generation_handle_event_ommeter;
            return;

        case VoltageGenerator:
            proccessor[HardwareFail].on_enter = proccessor_hardwarefail_on_enter_default;
            proccessor[HardwareFail].on_exit = proccessor_hardwarefail_on_exit_default;
            proccessor[HardwareFail].proccess = proccessor_hardwarefail_proccess_default;
            proccessor[HardwareFail].handle_event = proccessor_hardwarefail_handle_event_default;
            proccessor[Commutation].on_enter = proccessor_commutation_on_enter_probe; 
            proccessor[Commutation].on_exit = proccessor_commutation_on_exit;
            proccessor[Commutation].proccess = proccessor_commutation_proccess_default;
            proccessor[Commutation].handle_event = proccessor_commutation_handle_event_default;
            proccessor[Generation].on_enter = proccessor_generation_on_enter_generator;
            proccessor[Generation].on_exit = proccessor_generation_on_exit_generator;
            proccessor[Generation].proccess = proccessor_generation_proccess_generator;
            proccessor[Generation].handle_event = proccessor_generation_handle_event_generator;
            return;

        default: return;
    }
}
#endif