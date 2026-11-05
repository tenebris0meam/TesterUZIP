#ifndef GENERATION_H_
#define GENERATION_H_

#include "proccessor.h"
#include "model.h"

extern int32_t voltage_max_uV;
extern int32_t voltage_max_buf[MAX_TESTS_FOR_ONE_GDT];
extern volatile uint8_t test_id;

void proccessor_generation_on_enter_checkdevice (void);
void proccessor_generation_on_exit_checkdevice_default (void);
void proccessor_generation_proccess_checkdevice_default (void);
State proccessor_generation_handle_event_checkdevice_default (Event);

void proccessor_generation_proccess_checkdevice_for_GDT (void);
State proccessor_generation_handle_event_checkdevice_for_GDT (Event);
void proccessor_generation_on_exit_checkdevice_for_GDT (void);

typedef enum {
    Ommeter_Invalid,
    Ommeter_Valid,
    Ommeter_Error
} OmmeterStatus;

void proccessor_generation_on_enter_voltmeter (void);
void proccessor_generation_on_exit_voltmeter (void);
void proccessor_generation_proccess_voltmeter (void);
State proccessor_generation_handle_event_voltmeter (Event);

void proccessor_generation_on_enter_ommeter (void);
void proccessor_generation_on_exit_ommeter (void);
void proccessor_generation_proccess_ommeter (void);
State proccessor_generation_handle_event_ommeter (Event);
OmmeterStatus proccessor_ommeter_get_status (void);
int32_t proccessor_ommeter_get_resistance_ohm (void);

void proccessor_set_manual_target_voltage (uint16_t volts);
uint16_t proccessor_get_manual_target_voltage (void);
void proccessor_generation_on_enter_generator (void);
void proccessor_generation_on_exit_generator (void);
void proccessor_generation_proccess_generator (void);
State proccessor_generation_handle_event_generator (Event);

#endif /* GENERATION_H_ */