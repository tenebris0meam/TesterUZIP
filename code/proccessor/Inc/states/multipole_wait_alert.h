#ifndef MULTIPOLE_WAIT_ALERT_H_
#define MULTIPOLE_WAIT_ALERT_H_

#include "proccessor.h"

/* Значение 20U не работает */
#define CCR_VAL_MULTIPOLE (21U)

void proccessor_multipole_wait_alert_on_enter(void);
void proccessor_multipole_wait_alert_on_exit(void);
void proccessor_multipole_wait_alert_process(void);
State proccessor_multipole_wait_alert_handle_event(Event event);

#endif /* MULTIPOLE_WAIT_ALERT_H_ */
