#ifndef HARDWARE_FAIL_H_
#define HARDWARE_FAIL_H_

#include "proccessor.h"

void proccessor_hardwarefail_on_enter_default (void);
void proccessor_hardwarefail_on_exit_default (void);
void proccessor_hardwarefail_proccess_default (void);
State proccessor_hardwarefail_handle_event_default (Event);

void proccessor_hardwarefail_on_enter_for_GDT (void);
void proccessor_hardwarefail_on_exit_for_GDT (void);
void proccessor_hardwarefail_proccess_for_GDT (void);
State proccessor_hardwarefail_handle_event_for_GDT (Event);

#endif /* HARDWARE_FAIL_H_ */