#ifndef FLASH_H_
#define FLASH_H_

#include <stdbool.h>
#include <stdint.h>
#include "stm32f103xb.h"
#include "stm32f1xx.h"

#define FLASH_PAGE_LEN (0x0400UL)

typedef enum {
    Flash_OK, 
    Flash_Busy, 
    Flash_ErrorProgramming,
    Flash_ErrorWriteProtection,
    Flash_ErrorLocked, 
    Flash_ErrorInvalidAlign, 
    Flash_ErrorInvalidAddress, 
    Flash_Timeout
} FlashStatus;

FlashStatus flash_unlock (void);
FlashStatus flash_lock (void);
FlashStatus flash_wait_operation_ends (uint32_t timeout);
FlashStatus flash_erase_page (uint8_t page_id, uint32_t timeout);
FlashStatus flash_write16 (void *address, uint16_t data, uint32_t timeout);
FlashStatus flash_write_buffer (void *address, void *data, uint32_t length, uint32_t timeout);
void flash_read_buffer (void *address, void *data, uint32_t length);
bool flash_is_page_erased (uint8_t page_id);

static inline uint16_t flash_read16 (void *address) {
    return *(volatile uint16_t *)address;
}

static inline uint32_t flash_get_status_flags (void) {
    return READ_REG(FLASH->SR);
}

static inline void flash_clear_status_flags (void) {
    CLEAR_BIT(FLASH->SR, FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR);
    return;
}

static inline uint8_t flash_get_page_number (void *address) {
    return (uint8_t)(((uint32_t)address - FLASH_BASE) / FLASH_PAGE_LEN);
}

static inline void *flash_get_address (uint8_t page_id) {
    return (void *)(FLASH_BASE + page_id * FLASH_PAGE_LEN);
}

#endif /* FLASH_H_ */