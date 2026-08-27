#ifndef TIM_H
#define TIM_H

#include <stdbool.h>
#include <stdint.h>

bool TIM1_Init (void);
void TIM1_Start_PWM (void);
void TIM1_Stop_PWM (void);
void TIM1_set_CCR_value (uint32_t CCR_val);
uint32_t TIM1_get_clock_frequency (void);
uint32_t TIM1_get_PWM_frequency (void);
bool TIM1_set_PWM_frequency (uint32_t new_PWM_freq);

bool TIM3_Init (void);
void TIM3_Start_Count (void);
void TIM3_Stop_Count (void);
void TIM3_Change_Update_Time (uint32_t new_update_time_us);

#endif /* TIM_H */