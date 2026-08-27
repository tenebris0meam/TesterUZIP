#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f103xb.h"
#include "stm32f1xx_ll_gpio.h"

bool GPIO_Init (void);

typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
} Pin_TypeDef;

extern const Pin_TypeDef BUZZER;
extern const Pin_TypeDef nCS_MON;
extern const Pin_TypeDef nALERT;
extern const Pin_TypeDef nCS_LCD;
extern const Pin_TypeDef LCD_ADDR0;
extern const Pin_TypeDef nRESET_LCD;
extern const Pin_TypeDef HV_GEN_PWM;
extern const Pin_TypeDef nMUX1_EN;
extern const Pin_TypeDef MUX1_A;
extern const Pin_TypeDef MUX1_B;
extern const Pin_TypeDef MUX1_C;
extern const Pin_TypeDef nMUX2_EN;
extern const Pin_TypeDef MUX2_A;
extern const Pin_TypeDef MUX2_B;
extern const Pin_TypeDef MUX2_C;
extern const Pin_TypeDef PROBE_EN;
extern const Pin_TypeDef DSUB_EN;
extern const Pin_TypeDef RJ_SHIELD_P;
extern const Pin_TypeDef RJ_SHIELD_N;



static inline void Pin_Set_State (const Pin_TypeDef *pin, bool state) {
    return (state ? 
            LL_GPIO_SetOutputPin(pin->port, pin->pin) : 
            LL_GPIO_ResetOutputPin(pin->port, pin->pin));
}

static inline bool Pin_Get_State (const Pin_TypeDef *pin) {
    return (bool)(LL_GPIO_ReadInputPort(pin->port) & pin->pin);
}

#endif /* GPIO_H */