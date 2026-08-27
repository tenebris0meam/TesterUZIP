#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stdbool.h>

#define KEYBOARD_VOLTAGE_LSB_uV (806UL)

extern volatile uint16_t adc_buf;

bool ADC1_Init (void);
void ADC1_Start (void);
uint32_t ADC1_get_keyboard_voltage_uV (void);

#endif /* ADC_H */