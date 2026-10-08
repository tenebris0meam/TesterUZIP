#ifndef TESTFAILED_H_
#define TESTFAILED_H_

#include "proccessor.h"

void proccessor_testfailed_on_enter (void);
void proccessor_testfailed_on_exit (void);
void proccessor_testfailed_proccess (void);
State proccessor_testfailed_handle_event (Event);

#endif /* TESTFAILED_H_ */