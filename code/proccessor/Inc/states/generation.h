#ifndef GENERATION_H_
#define GENERATION_H_

#include "proccessor.h"

void proccessor_generation_on_enter (void);
void proccessor_generation_on_exit (void);
void proccessor_generation_proccess (void);
State proccessor_generation_handle_event (Event);

#endif /* GENERATION_H_ */