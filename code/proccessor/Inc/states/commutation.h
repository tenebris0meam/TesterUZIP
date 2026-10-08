#ifndef COMMUTATION_H_
#define COMMUTATION_H_

#include "proccessor.h"

void proccessor_commutation_on_enter (void);
void proccessor_commutation_on_exit (void);
void proccessor_commutation_proccess (void);
State proccessor_commutation_handle_event (Event);

#endif /* COMMUTATION_H_ */