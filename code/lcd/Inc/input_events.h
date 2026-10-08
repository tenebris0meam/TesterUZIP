#ifndef INPUT_EVENTS_H_
#define INPUT_EVENTS_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    InputEvent_HelloScreenTimeout,
    InputEvent_ButtonPressed_Previous, 
    InputEvent_ButtonPressed_Cancel, 
    InputEvent_ButtonPressed_Next, 
    InputEvent_ButtonPressed_Enter, 
    InputEvent_TestSequenceComplete,
    InputEvent_Count
} InputEvent;

void inputevent_enable_init (void);
void inputevent_allow_input (void);
void inputevent_deny_input (void); 
bool inputevent_get_input (void);
void inputevent_testing_completed (void);

InputEvent inputevent_get_state (void);
void inputevent_routine (void);

#endif /* INPUT_EVENTS_H_ */