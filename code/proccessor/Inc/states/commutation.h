#ifndef COMMUTATION_H_
#define COMMUTATION_H_

#include "proccessor.h"

void proccessor_commutation_on_enter_checkdevice (void);
void proccessor_commutation_on_enter_probe (void);
void proccessor_commutation_on_exit (void);
void proccessor_commutation_proccess_default (void);
State proccessor_commutation_handle_event_default (Event);

#endif /* COMMUTATION_H_ */