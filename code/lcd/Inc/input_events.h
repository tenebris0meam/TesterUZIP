#ifndef INPUT_EVENTS_H_
#define INPUT_EVENTS_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    InputEvent_HelloScreenTimeout,
    InputEvent_ButtonPressed_Previous, 
    InputEvent_ButtonPressed_Cancel, 
    InputEvent_ButtonTripplePressed_Cancel,
    InputEvent_ButtonPressed_Next, 
    InputEvent_ButtonPressed_Enter, 
    InputEvent_TestSequenceComplete,
    InputEvent_TestDeniedScreenTimeout,
    InputEvent_SystemEvent,
    InputEvent_HardwareFail,
    InputEvent_Count
} InputEvent;

void inputevent_testing_completed (void);
InputEvent inputevent_get_state (void);
void inputevent_routine (void);
void inputevent_enable_init (void);
void inputevent_enable_systemevent (void);
void inputevent_enable_hardwarefail (void);
void inputevent_enable_testdenied (void);

#endif /* INPUT_EVENTS_H_ */