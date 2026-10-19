#ifndef EXTI_H_
#define EXTI_H_

#include <stdbool.h>

typedef enum {
    UP = (1UL << 0),
    REJECT = (1UL << 1),
    DOWN = (1UL << 2),
    ACCEPT = (1UL << 3)
} Button_TypeDef;

bool EXTI_Init (void);
void EXTI_enable_interrupt (void);
void EXTI_disable_interrupt (void);

#endif /* EXTI_H_ */