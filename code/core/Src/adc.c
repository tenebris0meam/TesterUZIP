#include "adc.h"
#include "main.h"
#include "stm32f103xb.h"
#include "stm32f1xx.h"
#include "stm32f1xx_ll_adc.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_bus.h"

#include <stdbool.h>

volatile uint16_t adc_buf = 0;

bool ADC1_Init (void) {
    /* Включаем тактирование ADC1 */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);

    /* Настраиваем пин KEYBOARD, как аналоговый вход */
    LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = KEYBOARD_GPIO_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    if (LL_GPIO_Init(KEYBOARD_GPIO_Port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    /* Настраиваем параметры ADC1 */
    LL_ADC_InitTypeDef ADC_InitStruct = { 0 };
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.SequencersScanMode = LL_ADC_SEQ_SCAN_DISABLE;
    if (LL_ADC_Init(ADC1, &ADC_InitStruct) != SUCCESS) {
        return false;
    }

    /* Настраиваем канал 4 (PA4) */
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = { 0 };
    ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
    ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS;
    ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_UNLIMITED;
    if (LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct) != SUCCESS) {
        return false;
    }
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_4);

    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_4, LL_ADC_SAMPLINGTIME_239CYCLES_5);

    LL_ADC_Enable(ADC1);
    /* Задержка стабилизации АЦП (параметр t_STAB из даташита) */
    // volatile для отключения оптимизации
    for (volatile uint16_t i = 0; i < 1000; i++) {}

    LL_ADC_StartCalibration(ADC1);
    while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0) {}

    return true;
}

void ADC1_Start (void) {
    LL_ADC_REG_StartConversionSWStart(ADC1);
    return;
}

uint32_t ADC1_get_keyboard_voltage_uV (void) {
    return ((uint32_t)adc_buf * KEYBOARD_VOLTAGE_LSB_uV);
}