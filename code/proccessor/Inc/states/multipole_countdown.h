#ifndef MULTIPOLE_COUNTDOWN_H_
#define MULTIPOLE_COUNTDOWN_H_

#include "proccessor.h"

void proccessor_multipole_countdown_on_enter(void);
void proccessor_multipole_countdown_on_exit(void);
void proccessor_multipole_countdown_process(void);
State proccessor_multipole_countdown_handle_event(Event event);

#endif /* MULTIPOLE_COUNTDOWN_H_ */
