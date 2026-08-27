#ifndef DELAY_H_
#define DELAY_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    Millis,
    Micros
} DelayTimeType;

void delay_init (uint32_t SystemCoreClock_Hz);
bool delay_register_task (uint8_t *task_id, uint32_t delay, DelayTimeType type_delay);
bool delay_unregister_task (uint8_t task_id);
bool delay_is_not_yet_expired (uint8_t task_id);
void delay_ms (uint32_t ms);
void delay_us (uint32_t us);

#endif /* DELAY_H_ */