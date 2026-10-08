#ifndef IDLE_SETTING_H_
#define IDLE_SETTING_H_

#include "proccessor.h"

void proccessor_idle_on_enter (void);
void proccessor_idle_on_exit (void);
void proccessor_idle_proccess (void);
State proccessor_idle_handle_event (Event);

#endif /* IDLE_SETTING_H_ */