#ifndef HARDWARE_FAIL_H_
#define HARDWARE_FAIL_H_

#include "proccessor.h"

void proccessor_hardwarefail_on_enter (void);
void proccessor_hardwarefail_on_exit (void);
void proccessor_hardwarefail_proccess (void);
State proccessor_hardwarefail_handle_event (Event);

#endif /* HARDWARE_FAIL_H_ */