#ifndef CRITICAL_SECTION_H_
#define CRITICAL_SECTION_H_

#include "cmsis_compiler.h"
#include <stdbool.h>

extern volatile uint32_t mask;

static inline void enter_critsec (void) {
    mask = __get_PRIMASK();
    __disable_irq();
    return;
}

static inline void exit_critsec (void) {
    __set_PRIMASK(mask);
    return;
}

#endif /* CRITICAL_SECTION_H_ */