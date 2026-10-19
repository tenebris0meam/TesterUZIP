#include "exti.h"
#include "main.h"
#include "stm32f103xb.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_exti.h"



bool EXTI_Init (void) {
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);

    LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = BUTTON_UP_GPIO_Pin | BUTTON_DOWN_GPIO_Pin | 
                            BUTTON_REJECT_GPIO_Pin | BUTTON_ACCEPT_GPIO_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    if (LL_GPIO_Init(BUTTON_UP_GPIO_Port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    /* 3. Подключение PA0..PA3 к линиям EXTI0..EXTI3 */
    LL_GPIO_AF_SetEXTISource(LL_GPIO_AF_EXTI_PORTA,
                            LL_GPIO_AF_EXTI_LINE0);
    LL_GPIO_AF_SetEXTISource(LL_GPIO_AF_EXTI_PORTA,
                            LL_GPIO_AF_EXTI_LINE1);
    LL_GPIO_AF_SetEXTISource(LL_GPIO_AF_EXTI_PORTA,
                            LL_GPIO_AF_EXTI_LINE2);
    LL_GPIO_AF_SetEXTISource(LL_GPIO_AF_EXTI_PORTA,
                            LL_GPIO_AF_EXTI_LINE3);

    LL_EXTI_InitTypeDef EXTI_InitStruct = { 0 };
    EXTI_InitStruct.Line_0_31 = BUTTON_UP_GPIO_Pin | BUTTON_DOWN_GPIO_Pin | 
                                BUTTON_REJECT_GPIO_Pin | BUTTON_ACCEPT_GPIO_Pin;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
    if (LL_EXTI_Init(&EXTI_InitStruct) != SUCCESS) {
        return false;
    }

    NVIC_SetPriority(EXTI0_IRQn, 15);
    NVIC_SetPriority(EXTI1_IRQn, 15);
    NVIC_SetPriority(EXTI2_IRQn, 15);
    NVIC_SetPriority(EXTI3_IRQn, 15);
    return true;
}

void EXTI_enable_interrupt (void) {
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_ClearPendingIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_ClearPendingIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    return;
}

void EXTI_disable_interrupt (void) {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0 | 
                           LL_EXTI_LINE_1 | 
                           LL_EXTI_LINE_2 | 
                           LL_EXTI_LINE_3);
    NVIC_DisableIRQ(EXTI0_IRQn);
    NVIC_DisableIRQ(EXTI1_IRQn);
    NVIC_DisableIRQ(EXTI2_IRQn);
    NVIC_DisableIRQ(EXTI3_IRQn);
    return;
}