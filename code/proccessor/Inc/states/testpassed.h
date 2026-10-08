#ifndef TESTPASSED_H_
#define TESTPASSED_H_

#include "proccessor.h"

void proccessor_testpassed_on_enter (void);
void proccessor_testpassed_on_exit (void);
void proccessor_testpassed_proccess (void);
State proccessor_testpassed_handle_event (Event);

#endif /* TESTPASSED_H_ */