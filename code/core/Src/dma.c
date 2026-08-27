#include "dma.h"
#include "adc.h"
#include "stm32f103xb.h"
#include "stm32f1xx_ll_dma.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_spi.h"



bool DMA1_SPI2_Init (void) {
    /* Включаем тактирование DMA1 */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    LL_DMA_InitTypeDef DMA_InitStruct = { 0 };
    /* Настраиваем трансфер из ADC1 в память */
    DMA_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)&ADC1->DR;
    DMA_InitStruct.MemoryOrM2MDstAddress = (uint32_t)&adc_buf;
    DMA_InitStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    DMA_InitStruct.Mode = LL_DMA_MODE_CIRCULAR;
    DMA_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_NOINCREMENT;
    DMA_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_HALFWORD;
    DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_HALFWORD;
    DMA_InitStruct.NbData = 1;
    DMA_InitStruct.Priority = LL_DMA_PRIORITY_MEDIUM;
    if (LL_DMA_Init(DMA1, LL_DMA_CHANNEL_1, &DMA_InitStruct) != SUCCESS) {
        return false;
    }
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
    while (!LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_1)) {}

    return true;
}

bool DMA1_SPI2RX_Init (uint32_t where_to_store, uint32_t datanum) {
    /* Настраиваем трансфер из SPI2 в память (RX), канал 4 */
    LL_DMA_InitTypeDef DMA_InitStruct = { 0 };
    DMA_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)&SPI2->DR;
    DMA_InitStruct.MemoryOrM2MDstAddress = where_to_store;
    DMA_InitStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    DMA_InitStruct.Mode = LL_DMA_MODE_NORMAL;
    DMA_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_InitStruct.NbData = datanum;
    DMA_InitStruct.Priority = LL_DMA_PRIORITY_HIGH;
    if (LL_DMA_Init(DMA1, LL_DMA_CHANNEL_4, &DMA_InitStruct) != SUCCESS) {
        return false;
    }

    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_4);
    NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
    return true;
}

bool DMA1_SPI2RX_DeInit (void) {
    return (LL_DMA_DeInit(DMA1, LL_DMA_CHANNEL_4) == SUCCESS);
}

bool DMA1_SPI2TX_Init (uint32_t where_to_get, uint32_t datanum) {
    /* Настраиваем трансфер из памяти в SPI2 (TX), канал 5 */
    LL_DMA_InitTypeDef DMA_InitStruct = { 0 };
    DMA_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)&SPI2->DR;
    DMA_InitStruct.MemoryOrM2MDstAddress = where_to_get;
    DMA_InitStruct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_InitStruct.Mode = LL_DMA_MODE_NORMAL;
    DMA_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_InitStruct.NbData = datanum;
    DMA_InitStruct.Priority = LL_DMA_PRIORITY_HIGH;
    if (LL_DMA_Init(DMA1, LL_DMA_CHANNEL_5, &DMA_InitStruct) != SUCCESS) {
        return false;
    }

    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5);
    NVIC_SetPriority(DMA1_Channel5_IRQn, 1);
    return true;
}

bool DMA1_SPI2TX_DeInit (void) {
    return (LL_DMA_DeInit(DMA1, LL_DMA_CHANNEL_5) == SUCCESS);
}

void DMA1_SPI2RX_Start (void) {
    LL_SPI_EnableDMAReq_RX(SPI2);

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
    while (!LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {}
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);

    return;
}

void DMA1_SPI2TX_Start (void) {
    LL_SPI_EnableDMAReq_TX(SPI2);

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
    while (!LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5)) {}
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);

    return;
}

void DMA1_SPI2RX_Stop (void) {
	LL_SPI_DisableDMAReq_RX(SPI2);

	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
    while (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {}
    NVIC_DisableIRQ(DMA1_Channel4_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel4_IRQn);

    LL_DMA_ClearFlag_GI4(DMA1);
    return;
}

void DMA1_SPI2TX_Stop (void) {
	LL_SPI_DisableDMAReq_TX(SPI2);

	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
    while (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5)) {}
    NVIC_DisableIRQ(DMA1_Channel5_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel5_IRQn);

    LL_DMA_ClearFlag_GI5(DMA1);
    return;
}