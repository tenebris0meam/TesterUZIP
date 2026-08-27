#ifndef BUZZER_H_
#define BUZZER_H_

#include "gpio.h"

typedef struct {
    const Pin_TypeDef *const pin;
    const uint32_t beep_time_ms;
} Buzzer_TypeDef;

void buzzer_turn_on (void);
void buzzer_turn_off (void);
void buzzer_routine (void);
bool buzzer_is_beep_in_operation (void);

extern const Buzzer_TypeDef buzzer;

#endif /* BUZZER_H_ */