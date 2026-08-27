#ifndef DMA_H
#define DMA_H

#include <stdbool.h>
#include <stdint.h>

bool DMA1_SPI2_Init (void);
bool DMA1_SPI2RX_Init (uint32_t where_to_store, uint32_t datanum);
bool DMA1_SPI2RX_DeInit (void);
bool DMA1_SPI2TX_Init (uint32_t where_to_get, uint32_t datanum);
bool DMA1_SPI2TX_DeInit (void);
void DMA1_SPI2RX_Start (void);
void DMA1_SPI2TX_Start (void);
void DMA1_SPI2RX_Stop (void);
void DMA1_SPI2TX_Stop (void);

#endif /* DMA_H */