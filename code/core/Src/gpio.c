#include "gpio.h"
#include "main.h"
#include "stm32f1xx.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_bus.h"

#include <stdbool.h>

const Pin_TypeDef BUZZER = { .port = BUZZER_GPIO_Port, .pin = BUZZER_GPIO_Pin };
const Pin_TypeDef nCS_MON = { .port = nCS_MON_GPIO_Port, .pin = nCS_MON_GPIO_Pin };
const Pin_TypeDef nALERT = { .port = nALERT_GPIO_Port, .pin = nALERT_GPIO_Pin };
const Pin_TypeDef nCS_LCD = { .port = nCS_LCD_GPIO_Port, .pin = nCS_LCD_GPIO_Pin };
const Pin_TypeDef LCD_ADDR0 = { .port = LCD_ADDR0_GPIO_Port, .pin = LCD_ADDR0_GPIO_Pin };
const Pin_TypeDef nRESET_LCD = { .port = nRESET_LCD_GPIO_Port, .pin = nRESET_LCD_GPIO_Pin };
const Pin_TypeDef HV_GEN_PWM = { .port = HV_GEN_PWM_GPIO_Port, .pin = HV_GEN_PWM_GPIO_Pin };
const Pin_TypeDef nMUX1_EN = { .port = nMUX1_EN_GPIO_Port, .pin = nMUX1_EN_GPIO_Pin };
const Pin_TypeDef MUX1_A = { .port = MUX1_A_GPIO_Port, .pin = MUX1_A_GPIO_Pin };
const Pin_TypeDef MUX1_B = { .port = MUX1_B_GPIO_Port, .pin = MUX1_B_GPIO_Pin };
const Pin_TypeDef MUX1_C = { .port = MUX1_C_GPIO_Port, .pin = MUX1_C_GPIO_Pin };
const Pin_TypeDef nMUX2_EN = { .port = nMUX2_EN_GPIO_Port, .pin = nMUX2_EN_GPIO_Pin };
const Pin_TypeDef MUX2_A = { .port = MUX2_A_GPIO_Port, .pin = MUX2_A_GPIO_Pin };
const Pin_TypeDef MUX2_B = { .port = MUX2_B_GPIO_Port, .pin = MUX2_B_GPIO_Pin };
const Pin_TypeDef MUX2_C = { .port = MUX2_C_GPIO_Port, .pin = MUX2_C_GPIO_Pin };
const Pin_TypeDef PROBE_EN = { .port = PROBE_EN_GPIO_Port, .pin = PROBE_EN_GPIO_Pin };
const Pin_TypeDef DSUB_EN = { .port = DSUB_EN_GPIO_Port, .pin = DSUB_EN_GPIO_Pin };
const Pin_TypeDef RJ_SHIELD_P = { .port = RJ_SHIELD_P_GPIO_Port, .pin = RJ_SHIELD_P_GPIO_Pin };
const Pin_TypeDef RJ_SHIELD_N = { .port = RJ_SHIELD_N_GPIO_Port, .pin = RJ_SHIELD_N_GPIO_Pin };

bool GPIO_Init (void) {
    /* Включаем тактирвание портов A и B */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    /*
        PushPull:
        - PA2 (Buzzer)
        - PA10 (MUX1_C)
        - PB3 (MUX2_A)
        - PB4 (MUX2_B)
        - PB5 (MUX2_C)
        - PB7 (LCD_ADDR0)
        - PB10 (MUX1_A)
        - PB11 (MUX1_B)

        OpenDrain:
        - PA5 (RJ_SHIELD_P)
        - PA6 (Probe_En)
        - PA7 (DSUB_En)
        - PA9 (RJ_SHIELD_N)
        - PB0 (nMUX1_En)
        - PB1 (nMUX2_En)
        - PB8 (nRESET_LCD)
    */

    LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = MUX1_C.pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    if (LL_GPIO_Init(MUX1_C.port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    GPIO_InitStruct.Pin = MUX2_A.pin | 
                          MUX2_B.pin | 
                          MUX2_C.pin | 
                          LCD_ADDR0.pin | 
                          MUX1_A.pin | 
                          MUX1_B.pin;
    if (LL_GPIO_Init(MUX2_A.port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    GPIO_InitStruct.Pin = BUZZER.pin | 
                          RJ_SHIELD_P.pin | 
                          PROBE_EN.pin | 
                          DSUB_EN.pin | 
                          RJ_SHIELD_N.pin;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    if (LL_GPIO_Init(PROBE_EN.port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }
    LL_GPIO_SetOutputPin(BUZZER.port, BUZZER.pin);
    LL_GPIO_SetOutputPin(RJ_SHIELD_P.port, RJ_SHIELD_P.pin);
    LL_GPIO_SetOutputPin(RJ_SHIELD_N.port, RJ_SHIELD_N.pin);
    LL_GPIO_SetOutputPin(PROBE_EN.port, PROBE_EN.pin);
    LL_GPIO_SetOutputPin(DSUB_EN.port, DSUB_EN.pin);

    GPIO_InitStruct.Pin = nMUX1_EN.pin | 
                          nMUX2_EN.pin | 
                          nRESET_LCD.pin;
    if (LL_GPIO_Init(nMUX1_EN.port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }
    LL_GPIO_SetOutputPin(nMUX1_EN.port, nMUX1_EN.pin);
    LL_GPIO_SetOutputPin(nMUX2_EN.port, nMUX2_EN.pin);
    LL_GPIO_SetOutputPin(nRESET_LCD.port, nRESET_LCD.pin);

    return true;
}