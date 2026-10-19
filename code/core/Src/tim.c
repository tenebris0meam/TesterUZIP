#include "tim.h"
#include "main.h"
#include "stm32f103xb.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_tim.h"

#include <stdbool.h>



bool TIM1_Init (void) {
    /* Включаем тактирование TIM1 */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

    /* 
        PA8 -> TIM1 PWM Channel 1 
        PB12 -> TIM1 Break Input
    */

    /* Конфигурируем вход аппаратной остановки таймера */
    LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = nALERT_GPIO_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    if (LL_GPIO_Init(nALERT_GPIO_Port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    /* Конфигурируем выходной канал, как выход - для задания безопасного уровня */
    GPIO_InitStruct.Pin = HV_GEN_PWM_GPIO_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    if (LL_GPIO_Init(HV_GEN_PWM_GPIO_Port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    /* Устанавливаем безопасный для схемы логический уровень */
    LL_GPIO_ResetOutputPin(HV_GEN_PWM_GPIO_Port, HV_GEN_PWM_GPIO_Pin);

    /* Настраиваем таймер на генерацию пилы */
    LL_TIM_InitTypeDef TIM_InitStruct = { 0 };
    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.Autoreload = 999; // 60 кГц
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    if (LL_TIM_Init(TIM1, &TIM_InitStruct) != SUCCESS) {
        return false;
    }

    /* Настраиваем Channel 1 */
    LL_TIM_OC_InitTypeDef OC_InitStruct = { 0 };
    OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
    OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
    OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH; // включенное состояние - HIGH
    OC_InitStruct.CompareValue = 0;
    if (LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH1, &OC_InitStruct) != SUCCESS) {
        return false;
    }

    /* Настраиваем Break Input */
    LL_TIM_BDTR_InitTypeDef BDTR_InitStruct = { 0 };
    BDTR_InitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
    BDTR_InitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
    BDTR_InitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
    BDTR_InitStruct.DeadTime = 0;
    BDTR_InitStruct.BreakState = LL_TIM_BREAK_ENABLE;
    BDTR_InitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_LOW;
    if (LL_TIM_BDTR_Init(TIM1, &BDTR_InitStruct) != SUCCESS) {
        return false;
    }

    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH1);

    /* Задаём безопасный уровень выхода при отключении через MOE */
    LL_TIM_OC_SetIdleState(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCIDLESTATE_LOW);

    /* Загружаем конфигурацию в теневые регистры */
    LL_TIM_GenerateEvent_UPDATE(TIM1);

    /* Запускаем пилу */
    LL_TIM_SetCounter(TIM1, 0);
    LL_TIM_EnableCounter(TIM1);

    /* Только теперь можно перевести пин в AF */
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    if (LL_GPIO_Init(GPIOA, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    /* Выключаем прерывание по обновлению таймера и включаем аппаратную остановку счёта по пину ALERT */
    LL_TIM_ClearFlag_UPDATE(TIM1);
    LL_TIM_DisableIT_UPDATE(TIM1);
    NVIC_DisableIRQ(TIM1_UP_IRQn);

    LL_TIM_ClearFlag_BRK(TIM1);
    LL_TIM_EnableIT_BRK(TIM1);
    NVIC_SetPriority(TIM1_BRK_IRQn, 0);

    /* Не начинаем сравнение для генерации ШИМ, пока не будет дана команда */

    return true;
}

void TIM1_Start_PWM (void) {
    NVIC_ClearPendingIRQ(TIM1_BRK_IRQn);
    NVIC_EnableIRQ(TIM1_BRK_IRQn);
    LL_TIM_EnableAllOutputs(TIM1);
    return;
}

void TIM1_Stop_PWM (void) {
    NVIC_DisableIRQ(TIM1_BRK_IRQn);
    LL_TIM_DisableAllOutputs(TIM1);
    return;
}

void TIM1_set_CCR_value (uint32_t CCR_val) {
    LL_TIM_OC_SetCompareCH1(TIM1, CCR_val);
    return;
}

uint32_t TIM1_get_clock_frequency (void) {
    LL_RCC_ClocksTypeDef Clocks_Struct;
    LL_RCC_GetSystemClocksFreq(&Clocks_Struct);

    uint32_t timer_clk_freq = Clocks_Struct.PCLK2_Frequency;
    if (LL_RCC_GetAPB1Prescaler() != LL_RCC_APB1_DIV_1) {
        timer_clk_freq *= 2;
    }

    return (timer_clk_freq / (LL_TIM_GetPrescaler(TIM1) + 1));
}

bool TIM1_set_PWM_frequency (uint32_t new_PWM_freq) {
    if ((new_PWM_freq < 1000) || (new_PWM_freq > 200000)) {
        return false;
    }

    uint32_t timer_tick_freq = TIM1_get_clock_frequency();
    uint32_t new_ARR = timer_tick_freq / new_PWM_freq - 1;
    
    if ((new_ARR == 0) || (new_ARR > 0xFFFF)) {
        return false;
    }

    uint32_t old_ARR = TIM1->ARR;
    uint32_t old_CCR1 = TIM1->CCR1;

    uint32_t new_CCR1 = (old_ARR > 0) ? 
                        (uint32_t)(((uint64_t)old_CCR1 * new_ARR) / old_ARR) : 
                        0;
    
    TIM1->ARR = new_ARR;
    TIM1->CCR1 = new_CCR1;

    return true;
}

uint32_t TIM1_get_PWM_frequency (void) {
    return (TIM1_get_clock_frequency() / (LL_TIM_GetAutoReload(TIM1) + 1));
} 



bool TIM3_Init (void) {
    /* Включаем тактирование TIM3 */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

    LL_TIM_InitTypeDef TIM_InitStruct = { 0 };
    TIM_InitStruct.Prescaler = 29; // частота таймера 1 МГц
    TIM_InitStruct.Autoreload = 99; // обновление с частотой 10 кГц (100 мкс - 2 преобразования по 50 мкс)
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.RepetitionCounter = 0;
    if (LL_TIM_Init(TIM3, &TIM_InitStruct) != SUCCESS) {
        return false;
    }

    LL_TIM_ClearFlag_UPDATE(TIM3);
    LL_TIM_SetAutoReload(TIM3, 0);
    LL_TIM_EnableIT_UPDATE(TIM3);
    // Обрабатываем только тогда, когда нет других активных прерываний
    // потому что таймер генерирует запросы к INA239
    NVIC_SetPriority(TIM3_IRQn, 2);
    /* Не разрешаем счёт таймера, пока не будет сконфигурирован INA239 */
    return true;
}

void TIM3_Start_Count (void) {
    NVIC_ClearPendingIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(TIM3_IRQn);
    LL_TIM_EnableCounter(TIM3);
    return;
}

void TIM3_Stop_Count (void) {
    NVIC_DisableIRQ(TIM3_IRQn);
    LL_TIM_DisableCounter(TIM3);
    return;
}

void TIM3_Change_Update_Time (uint32_t new_update_time_us) {
    LL_TIM_SetAutoReload(TIM3, new_update_time_us - 1);
    return;
}
