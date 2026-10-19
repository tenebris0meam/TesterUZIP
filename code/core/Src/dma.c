#include "dma.h"
#include "stm32f103xb.h"
#include "stm32f1xx_ll_dma.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_spi.h"
#include <stdbool.h>
#include <stdint.h>
#include "critical_section.h"

uint32_t start_tick_dma;
uint32_t dma_delay_tick;

static inline void dma_update_watchdog (void) {
    start_tick_dma = DWT->CYCCNT;
    return;
}

static inline bool dma_is_expired_watchdog (void) {
    uint32_t cur_tick = DWT->CYCCNT;
    if (start_tick_dma > cur_tick) {
        return (UINT32_MAX - start_tick_dma + cur_tick > dma_delay_tick);
    } else {
        return (cur_tick - start_tick_dma > dma_delay_tick);
    }
}

bool DMA1_SPI2_Init (void) {
    /* Включаем тактирование DMA1 */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
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
    NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
    return true;
}

bool DMA1_SPI2RX_DeInit (void) {
    return (LL_DMA_DeInit(DMA1, LL_DMA_CHANNEL_4) == SUCCESS);
}

/** Данную функцию нужно применять только после DMA1_SPI2RX_Init */
void DMA1_SPI2RX_AllowInterrupt (void) {
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_4);
    LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_4);
    NVIC_ClearPendingIRQ(DMA1_Channel4_IRQn);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    return;
}

void DMA1_SPI2RX_DenyInterrupt (void) {
    NVIC_DisableIRQ(DMA1_Channel4_IRQn);
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_4);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_4);
    return;
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
    NVIC_SetPriority(DMA1_Channel5_IRQn, 1);
    return true;
}

bool DMA1_SPI2TX_DeInit (void) {
    return (LL_DMA_DeInit(DMA1, LL_DMA_CHANNEL_5) == SUCCESS);
}

/** Данную функцию нужно применять только после DMA1_SPI2RX_Init */
void DMA1_SPI2TX_AllowInterrupt (void) {
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5);
    LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_5);
    NVIC_ClearPendingIRQ(DMA1_Channel5_IRQn);
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    return;
}

void DMA1_SPI2TX_DenyInterrupt (void) {
    NVIC_DisableIRQ(DMA1_Channel5_IRQn);
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_5);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_5);
    return;
}

bool DMA1_SPI2RX_Start (void) {
    enter_critsec();
    dma_update_watchdog();
    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {
        /* DMA был включен два раза подряд, ошибка */
        exit_critsec();
        return false;
    } else {
        LL_SPI_EnableDMAReq_RX(SPI2);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
        while (!dma_is_expired_watchdog()) {
            if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {
                /* DMA канал включился */
                exit_critsec();
                return true;
            }
        }
        /* DMA канал не включился */
        exit_critsec();
        return false;
    }
}

bool DMA1_SPI2TX_Start (void) {
    enter_critsec();
    dma_update_watchdog();
    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5)) {
        /* DMA был включен два раза подряд, ошибка */
        exit_critsec();
        return false;
    } else {
        LL_SPI_EnableDMAReq_TX(SPI2);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
        while (!dma_is_expired_watchdog()) {
            if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5)) {
                /* DMA канал включился */
                exit_critsec();
                return true;
            }
        }
        /* DMA канал не включился */
        exit_critsec();
        return false;
    }
}

bool DMA1_SPI2RX_Stop (void) {
    enter_critsec();
    dma_update_watchdog();
    if (!LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {
        /* DMA был выключен два раза подряд, ок */
        exit_critsec();
        return true;
    } else {
        LL_SPI_DisableDMAReq_RX(SPI2);
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        while (!dma_is_expired_watchdog()) {
            if (!LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {
                /* DMA канал выключился */
                exit_critsec();
                return true;
            }
        }
        /* DMA канал не выключился */
        exit_critsec();
        return false;
    }
}

bool DMA1_SPI2TX_Stop (void) {
    enter_critsec();
    dma_update_watchdog();
    if (!LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5)) {
        /* DMA был выключен два раза подряд, ок */
        exit_critsec();
        return true;
    } else {
        LL_SPI_DisableDMAReq_TX(SPI2);
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        while (!dma_is_expired_watchdog()) {
            if (!LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5)) {
                /* DMA канал выключился */
                exit_critsec();
                return true;
            }
        }
        /* DMA канал не выключился */
        exit_critsec();
        return false;
    }
}