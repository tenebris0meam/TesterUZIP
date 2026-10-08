#ifndef TESTSEQUENCE_COMPLETE_H_
#define TESTSEQUENCE_COMPLETE_H_

#include "proccessor.h"

void proccessor_testsequencecomplete_on_enter (void);
void proccessor_testsequencecomplete_on_exit (void);
void proccessor_testsequencecomplete_proccess (void);
State proccessor_testsequencecomplete_handle_event (Event);

#endif /* TESTSEQUENCE_COMPLETE_H_ */